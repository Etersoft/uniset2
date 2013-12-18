#include <iostream>
#include <string>
#include <cstring>

using namespace std;

int main()
{
    string str;
    getline(cin, str);
    cout << "size of str=" << str.size() << ", strlen=" << strlen(str.c_str()) << endl;
}
