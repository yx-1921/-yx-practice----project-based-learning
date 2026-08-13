#include <limits>
#define _USE_MATH_DEFINES
#include <cmath>
#include <iostream>
#include <fstream>
#include <vector>
#include "geometry.h"

void save_bmp(const std::string& filename, const std::vector<vec3>& framebuffer, int width, int height) {
    // BMP 文件头（54 字节）
    unsigned char bmp_header[54] = {
        0x42, 0x4D,              // 'BM'
        0, 0, 0, 0,              // 文件大小（稍后填充）
        0, 0, 0, 0,              // 保留
        54, 0, 0, 0,             // 像素数据偏移
        40, 0, 0, 0,             // DIB 头大小
        0, 0, 0, 0,              // 宽度（稍后填充）
        0, 0, 0, 0,              // 高度（稍后填充）
        1, 0,                    // 颜色平面数
        24, 0,                   // 每像素位数（24-bit）
        0, 0, 0, 0,              // 压缩方式（0 = 无压缩）
        0, 0, 0, 0,              // 图像大小（稍后填充）
        0, 0, 0, 0,              // 水平分辨率
        0, 0, 0, 0,              // 垂直分辨率
        0, 0, 0, 0,              // 调色板颜色数
        0, 0, 0, 0               // 重要颜色数
    };

    // 填充宽高
    *(int*)(&bmp_header[18]) = width;
    *(int*)(&bmp_header[22]) = height;

    // 计算像素数据大小（每行 4 字节对齐）
    int row_size = (width * 3 + 3) & ~3;
    int image_size = row_size * height;
    *(int*)(&bmp_header[34]) = image_size;
    *(int*)(&bmp_header[2]) = 54 + image_size;

    std::vector<unsigned char> pixels(image_size, 0);

    // BMP 是从下往上存储的
    for (size_t j = 0; j < height; j++) {
        for (size_t i = 0; i < width; i++) {
            vec3 color = framebuffer[i + j * width];
            double max_val = std::max(color.x, std::max(color.y, color.z));
            if (max_val > 1.0) color = color * (1.0 / max_val);

            size_t idx = (height - 1 - j) * row_size + i * 3;
            // bgr格式
            pixels[idx + 0] = (unsigned char)(255 * std::max(0.0, std::min(1.0, color.z)));
            pixels[idx + 1] = (unsigned char)(255 * std::max(0.0, std::min(1.0, color.y)));
            pixels[idx + 2] = (unsigned char)(255 * std::max(0.0, std::min(1.0, color.x)));
        }
    }

    std::ofstream ofs(filename, std::ios::binary);
    ofs.write((char*)bmp_header, 54);
    ofs.write((char*)pixels.data(), image_size);
    ofs.close();
    std::cout << "Image saved to " << filename << std::endl;
}

void save_ppm(const std::string& filename, const std::vector<vec3>& framebuffer, int width, int height)
{
    std::ofstream ofs;
    ofs.open(filename);
    if (!ofs.is_open()) {
        std::cerr << "Error: Cannot open file out.ppm" << std::endl;
        return;
    }

    // PPM 文件头
    ofs << "P6\n" << width << " " << height << "\n255\n";

    // 按行写入像素数据（从上到下，从左到右）
    for (size_t j = 0; j < height; j++) {
        for (size_t i = 0; i < width; i++) {
            //const vec3& color = framebuffer[i + j * width];
            //    这叫 Tone Mapping（色调映射），它的作用是：
            //    当像素的某个颜色分量超过 1.0 时（比如高光过曝），整体缩放到[0, 1] 范围
            //    保持颜色比例不变，防止画面出现“纯白”区域
            vec3 color = framebuffer[i + j * width];
            double max_val = std::max(color.x, std::max(color.y, color.z));
            if (max_val > 1.0) {
                color = color * (1.0 / max_val);
            }
            // 将 [0, 1] 范围的颜色值映射到 [0, 255] 并转为 char
            ofs << (char)(255 * std::max(0.0, std::min(1.0, color.x)));
            ofs << (char)(255 * std::max(0.0, std::min(1.0, color.y)));
            ofs << (char)(255 * std::max(0.0, std::min(1.0, color.z)));
        }
    }

    ofs.close();
    std::cout << "Image saved to " << filename << std::endl;
}

struct Light {
    Light(const vec3& p, const float& i) : position(p), intensity(i) {}
    vec3 position;
    float intensity;
};

struct Material {
    Material(const float &r, const vec4& a, const vec3& color, const float& spec) : refractive_index(r), albedo(a), diffuse_color(color), specular_exponent(spec) {}
    Material() : refractive_index(1), albedo({1, 0, 0, 0}), diffuse_color(), specular_exponent() {}
    float refractive_index;
    vec4 albedo;
    vec3 diffuse_color;
    float specular_exponent;
};

struct Sphere {
    vec3 center;
    float radius;
    Material material;

    Sphere(const vec3& c, const float& r, const Material &m) : center(c), radius(r), material(m){}

    bool ray_intersect(const vec3& orig, const vec3& dir, float &t0) const {
        vec3 L = center - orig;
        float pL = L * dir;
        float d2 = L * L - pL * pL;
        if (d2 > radius * radius) return false;
        float leg = sqrtf(radius * radius - d2);    // 直角三角形：斜边Hypotenuse，直角边口语：leg
        t0 = pL - leg;
        float t1 = pL + leg;
        if (t0 < 0) t0 = t1;
        if (t0 < 0) return false;
        return true;
    }
};

vec3 reflect(const vec3& I, const vec3& N) {
    return I - N * 2.f * (I * N);
}

vec3 refract(const vec3& I, const vec3& N, const float& refractive_index) { // 折射定律
    float cosi = -std::max(-1., std::min(1., I * N));
    float etai = 1, etat = refractive_index;
    vec3 n = N;
    if (cosi < 0) { // if the ray is inside the object, swap the indices and invert the normal to get the correct result
        cosi = -cosi;
        std::swap(etai, etat); n = -N;
    }
    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? vec3{ 0, 0, 0 } : I * eta + n * (eta * cosi - sqrtf(k));
}

bool scene_intersect(const vec3& orig, const vec3& dir, const std::vector<Sphere>& spheres, vec3& hit, vec3& N, Material& material) {
    float spheres_dist = std::numeric_limits<float>::max();
    for (size_t i = 0; i < spheres.size(); i++) {
        float dist_i;
        if (spheres[i].ray_intersect(orig, dir, dist_i) && dist_i < spheres_dist) {
            spheres_dist = dist_i;
            hit = orig + dir * dist_i;
            N = normalized(hit - spheres[i].center);
            material = spheres[i].material;
        }
    }

    //return spheres_dist < 1000;
    
    // 添加平面
    float checkerboard_dist = std::numeric_limits<float>::max();
    if (fabs(dir.y) > 1e-3) {
        float d = -(orig.y + 4) / dir.y; // the checkerboard plane has equation y = -4
        vec3 pt = orig + dir * d;
        if (d > 0 && fabs(pt.x) < 10 && pt.z<-10 && pt.z>-30 && d < spheres_dist) {
            checkerboard_dist = d;
            hit = pt;
            N = vec3{ 0, 1, 0 };
            material.diffuse_color = (int(.5 * hit.x + 1000) + int(.5 * hit.z)) & 1 ? vec3{ 1, 1, 1 } : vec3{ 1, .7, .3 };
            material.diffuse_color = material.diffuse_color * .3;
        }
    }
    return std::min(spheres_dist, checkerboard_dist) < 1000;
}

vec3 cast_ray(const vec3& orig, const vec3& dir, const std::vector<Sphere> &spheres, const std::vector<Light>& lights, size_t depth = 0) {
    vec3 point, N;
    Material material;

    if (depth > 4 || !scene_intersect(orig, dir, spheres, point, N, material)) {
        return vec3{ 0.2, 0.7, 0.8 }; // background color
    }
    // reflect
    vec3 reflect_dir = normalized(reflect(dir, N));
    vec3 reflect_orig = reflect_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3; // offset the original point to avoid occlusion by the object itself
    vec3 reflect_color = cast_ray(reflect_orig, reflect_dir, spheres, lights, depth + 1);
    // refract
    vec3 refract_dir = normalized(refract(dir, N, material.refractive_index));
    vec3 refract_orig = refract_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3;
    vec3 refract_color = cast_ray(refract_orig, refract_dir, spheres, lights, depth + 1);
    // end

    float diffuse_light_intensity = 0, specular_light_intensity = 0;
    for (size_t i = 0; i < lights.size(); i++) {
        vec3 light_dir = normalized(lights[i].position - point);
        // shadows
        float light_distance = norm(lights[i].position - point);
        vec3 shadow_orig = light_dir * N < 0 ? point - N * 1e-3 : point + N * 1e-3; // checking if the point lies in the shadow of the lights[i]
        vec3 shadow_pt, shadow_N;
        Material tmpmaterial;
        if (scene_intersect(shadow_orig, light_dir, spheres, shadow_pt, shadow_N, tmpmaterial) && norm(shadow_pt - shadow_orig) < light_distance)
            continue;
        // shadows end
        diffuse_light_intensity += lights[i].intensity * std::max(0., light_dir * N);
        specular_light_intensity += powf(std::max(0., -reflect(-light_dir, N) * dir), material.specular_exponent) * lights[i].intensity;
    }
    return material.diffuse_color * diffuse_light_intensity * material.albedo[0]
            + vec3{ 1., 1., 1. } *specular_light_intensity * material.albedo[1]
            + reflect_color * material.albedo[2]
            + refract_color * material.albedo[3];
}

void render(const std::vector<Sphere> &spheres, const std::vector<Light>& lights) {
    const int width = 1024;
    const int height = 768;
    const int fov = M_PI / 2.;
    std::vector<vec3> framebuffer(width * height);

#pragma omp parallel for
    for (size_t j = 0; j < height; j++) {
        for (size_t i = 0; i < width; i++) {
            float x = (2 * (i + 0.5) / (float)width - 1) * tan(fov / 2.) * width / (float)height;
            float y = -(2 * (j + 0.5) / (float)height - 1) * tan(fov / 2.);
            vec3 dir = normalized(vec3{ x, y, -1 });
            framebuffer[i + j * width] = cast_ray(vec3{ 0, 0, 0 }, dir, spheres, lights);
        }
    }
    
    // linux && windwos
    save_bmp("out.bmp", framebuffer, width, height);
    // only linux
    save_ppm("out.ppm", framebuffer, width, height);
}

int main() {
    Material      ivory(1.0, vec4{ 0.6,  0.3, 0.1, 0.0 }, vec3{ 0.4, 0.4, 0.3 }, 50.);
    Material      glass(1.5, vec4{ 0.0,  0.5, 0.1, 0.8 }, vec3{ 0.4, 0.4, 0.3 }, 125.);
    Material red_rubber(1.0, vec4{ 0.9,  0.1, 0.0, 0.0 }, vec3{ 0.3, 0.1, 0.1 }, 10.);
    Material     mirror(1.0, vec4{ 0.0, 10.0, 0.8, 0.0 }, vec3{ 1.0, 1.0, 1.0 }, 1425.);

    std::vector<Sphere> spheres;
    spheres.push_back(Sphere(vec3{-3,     0,   -16 }, 2,      ivory));
    spheres.push_back(Sphere(vec3{-1.0,  -1.5, -12 }, 2,      glass));
    spheres.push_back(Sphere(vec3{ 1.5,  -0.5, -18 }, 3, red_rubber));
    spheres.push_back(Sphere(vec3{ 7,     5,   -18 }, 4,     mirror));

    std::vector<Light>  lights;
    lights.push_back(Light(vec3{-20, 20,  20 }, 1.5));
    lights.push_back(Light(vec3{ 30, 50, -25 }, 1.8));
    lights.push_back(Light(vec3{ 30, 20,  30 }, 1.7));

    render(spheres, lights);
    return 0;
}