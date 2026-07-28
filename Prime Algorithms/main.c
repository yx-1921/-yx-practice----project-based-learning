#1 brute-forcing
int is_prime(const int x)
{
    if (x < 2) return -1;
    if (x < 4) return 1;
    if (x % 2 == 0) return 0;   // 跳过偶数
    int limit = (int)sqrt((double) x);
    for (int i = 3; i < limit; i += 2)  //跳过偶数
    {
        if (x % i == 0)
            return 0;
    }
    return 1;
}

#2 optimize 6k±1(x > 3时)
// 6k	    能被 6 整除，是合数
// 6k + 1	可能是素数
// 6k + 2	能被 2 整除，是合数
// 6k + 3	能被 3 整除，是合数
// 6k + 4	能被 2 整除，是合数
// 6k + 5   可能是素数（等价于 6k - 1）

bool is_prime(int x) {
    if (x < 2) return false;
    if (x == 2 || x == 3) return true;
    if (x % 2 == 0 || x % 3 == 0) return false;

    int limit = (int)sqrt((double)x);
    for (int i = 5; i <= limit; i += 6) {
        if (x % i == 0 || x % (i + 2) == 0) return false;
    }
    return true;
}

#3 埃氏筛法
void eratosthenes(int N) 
{
    bool is_prime[N + 1];
    for (int i = 2; i <= N; i++) is_prime[i] = true;

    for (int i = 2; i * i <= N; i++)
    {
        if (is_prime[i]) {
            // 从i * i开始，因为i*k,(k<i)的情况已经被标记过了，k要么是素数，要么含有更小的素数因子
            // i == 5的情况，i*2,被素数2标记， i*3被素数3标记，i*4倍素数2标记
            for (int j = i * i; j <= N; j += i)
            {
                is_prime[j] = false;
            }
        }
    }
    // 所有 is_prime[i] == true 的就是素数
}

#4 欧拉筛法（线性筛）：优化了埃氏筛法对合数的重复筛选，比如12会被2和3各标记一次
// 欧拉筛保证每个合数只被它的最小质因子标记一次。
void euler_sieve(int N)
{
    // is_prime[i] 标记合数：true 表示已被标记为合数，false 表示尚未被标记（质数或未处理）
    bool is_prime[N+1]; //全部为0
    // 存储素数
    int primes[N+1], count;

    // 从2开始遍历，2是最小的素数
    for (int i=2; i<N; i++)
    {
        // 2所在的位置为初始条件，并且添加到素数表中
        if (!is_prime[i])
            primes[count++] = i;
        for (int j=0; j<count; j++)
        {
            int x = i * primes[j];
            if (x > N) break;

            is_prime[x] = true; // 标记对应位置的合数
            // 剪枝：
            // 优化的关键，确保每个合数只被最小质因子标记一次
            if (i % primes[j] == 0)
                break;
        }
    }
}