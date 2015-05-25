#include<iostream>
#include<string>
#include<Magick++.h>
#include<fstream>
#include<boost/filesystem.hpp>


using namespace std;
using namespace boost::filesystem;

int main(int argc, char *argv[])
{
	if(2 != argc) {
			cout<<"Usage: exec picture name..."<<endl;
	}

	Magick::InitializeMagick(NULL);
	ifstream source(argv[1]);
	path file = argv[1];
	string content(istreambuf_iterator<char>(source), (istreambuf_iterator<char>()));
	source.close();

	path new_path = file.parent_path();
	string exten = file.extension().string();
	string base_name = basename(file) +"_thumb";
	new_path = (new_path/base_name).string() + exten;
	cout<<"base:"<<basename(file)<<endl;
	cout<<"exten:"<<exten<<endl;
	cout<<"thumbnail:"<<new_path.string()<<endl;

	Magick::Blob block(content.c_str(), content.length());

	Magick::Image obj(block);

	obj.sample(Magick::Geometry("230x230"));
	string filename = new_path.string();
	obj.write(filename);
	return 0;
}
