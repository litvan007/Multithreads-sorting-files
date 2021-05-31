#include <iostream>
#include <fstream>

using namespace std;


void readFile(ifstream &fin){
    int n = 0;
    fin >> n;
    int32_t nums[n];
    fin.read((char*)nums, n*sizeof(int32_t));
    fin.close();
    for (int i = 0; i<n; ++i){
        cout << nums[i] << endl;
    }
    cout << endl;
}

int main(){
    for (int i = 0; i < 1; ++i){
        ifstream fin("./test_file.bin");
        if (!fin.is_open()){
            cout << "File "+to_string(i) << " -- Read error!" << endl;
        }
        else{
            readFile(fin);
        }
    }
}
