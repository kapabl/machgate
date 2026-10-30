int darwin_asan_global = 21;

int main(void)
{
    volatile char buffer[16];
    for (int i = 0; i < 16; ++i)
        buffer[i] = (char)(i + 1);
    int sum = 0;
    for (int i = 0; i < 16; ++i)
        sum += buffer[i];
    return sum + darwin_asan_global == 157 ? 42 : 1;
}
