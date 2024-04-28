// 可追加putint函数的原型，避免静态检查或编译警告的产生
// 如果不追加原型，编译时需通过--include选型指定头文件忽略警告

int putint(int a, int b);
int g = 3;
int main()
{
    int b;
    int a = b + 3;

    putint(b);

    return 0;
}