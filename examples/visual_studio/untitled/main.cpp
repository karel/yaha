#include <iostream>
#include "../../../include/hidapi.h"

using namespace std;

int main(int ac, char** av)
{
	HidApi m_hid;
	for (auto &x : m_hid.m_devices)
		wcout << (x.second)->getProduct() << endl;

	char c = getchar();
}