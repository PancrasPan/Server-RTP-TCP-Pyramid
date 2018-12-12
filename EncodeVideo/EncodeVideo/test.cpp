#include<iostream>
#include<string>
#include<fstream>
using namespace std;
int main() {
	ifstream fin;
	fin.open("test1.txt", ios::in);
	if (!fin) {
		cout << "打开文件失败！" << endl;
	}
	fin.close();
	fin.open("test.txt", ios::in);
	if (!fin) {
		cout << "打开文件失败！" << endl;
	}	
	string a;
	fin >> a;
	cout << a << endl;
	fin.close();
	return 0;

}