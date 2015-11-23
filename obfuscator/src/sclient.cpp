#include <iostream>
#include <fstream>
#include <cstdlib>

#define DEBUG_MODE

#ifdef DEBUG_MODE
#define INFO(msg) cout << "INFO: " << msg << "\n";
#else
#define INFO(msg) 
#endif

#define CHECK_ERROR(res, call, fn) if (res != 0) { cout << "ERROR: " << fn << " returned " << res << " in " << call << "\n"; exit(res);};



using namespace std;

/* creates a randomly generated file of a specified size*/
int generate_dummy_file(int f_size, string f_name, string path);
int main()
{
	int res = 0; 
	string f_name = "test_file.bin";
	string f_path = "./";
	int f_size = 1000;
	
	res = generate_dummy_file(f_size, f_name, f_path);
	CHECK_ERROR(res, "main()", "generate_dummy_file(f_size, f_name, f_path)");
	
	INFO(res);
}

int generate_dummy_file(int f_size, string f_name, string path)
{
	INFO("generate_dummy_file: size " << f_size << ", name " << f_name << ", path " << path);
	char buffer[f_size];
	for (int i = 0; i < f_size; i++)
	{
		char rand_char = (char) (rand() % 256);
		buffer[i] = rand_char;
	}
	ofstream myfile (f_name.c_str(), ios::out | ios::app | ios::binary);
	myfile.write (buffer, f_size);
	myfile.close();

	return 0;
}