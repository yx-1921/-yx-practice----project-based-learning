#include <vector>
#include <iostream>

class A {
public:
    A()
        : a(10)
    {}
    int a;
};

const int SIZE = 10;
A *arr = new A[SIZE];
int *p = (int*)malloc(sizeof(int) * SIZE);

int main() {
    for (int i = 0; i < SIZE; i++)
        p[i] = i;
    for (int i = 0; i < SIZE; i++)
        std::cout << "p[" << i << "] == " << p[i] << std::endl;
    for (int i = 0; i < SIZE; i++)
        arr[i].a = i;
    for (int i = 0; i < SIZE; i++)
        std::cout << " arr[" << i << "].a == " << arr[i].a <<std::endl;

    free(p);
    p = nullptr;
}