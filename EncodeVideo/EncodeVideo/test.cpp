#include<iostream>
#include<string>
#include<fstream>
using namespace std;
int main() {
	ifstream fin;
	fin.open("test1.txt", ios::in);
	if (!fin) {
		cout << "���ļ�ʧ�ܣ�" << endl;
	}
	fin.close();
	fin.open("test.txt", ios::in);
	if (!fin) {
		cout << "���ļ�ʧ�ܣ�" << endl;
	}	
	string a;
	fin >> a;
	cout << a << endl;
	fin.close();
	return 0;

}