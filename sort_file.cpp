#include <iostream>
#include <fstream>
#include <algorithm>
#include <mutex>
#include <thread>
#include <numeric>
#include <vector>
#include <queue>
#include <condition_variable>
#include <time.h>
#include <stdexcept>
#include <cmath>

using namespace std;

class SafeQueue{
	queue<string> q;
	condition_variable c;
	mutable mutex m;
	mutable mutex vector_m;
	vector<bool> everybody_works;

public:
	SafeQueue(int thread_num){
		everybody_works.assign(thread_num, false);
	}
	
	SafeQueue(SafeQueue const& other){
		lock_guard<mutex> g(other.m);
		lock_guard<mutex> gv(other.vector_m);
		q = other.q;
		everybody_works = other.everybody_works;
	}
		
	SafeQueue(queue<string> const& default_queue, int thread_num){
		q = default_queue;
		everybody_works.assign(thread_num, false);
	}

	void push(string val){
		lock_guard<mutex> g(m);
		q.push(val);
		c.notify_one();
	}
		
	int size(){
		lock_guard<mutex> g(m);
		return q.size();
	}
	
	void set_me_working(int th, bool val){
		lock_guard<mutex> g(vector_m);
		everybody_works[th] = val;
		c.notify_one();
	}
		
	bool is_everybody_working(){
		lock_guard<mutex> g(vector_m);
		return accumulate(everybody_works.begin(), everybody_works.end(), false);
	}
	
	string just_pop(){
		lock_guard<mutex> lk(m);
		if(q.empty())
			throw "No elems";
		string a = q.front();
		q.pop();
		return a;
	}
	
	bool wait_pop(string& a, string& b){
		unique_lock<mutex> lk(m);
		c.wait(lk, [this]{ return q.size()>1 || !is_everybody_working();});
		if(q.empty()){
			throw "Ooops! Smth has gone wrong!";
		}
		if(q.size()==1) 
			return false; 
		a = q.front();
		q.pop();
		b = q.front();
		q.pop();
		return true;
    }
};

void start_sort(int32_t* arr, int n, int j, SafeQueue &q, mutex &m){
    ofstream out;
    m.lock();
    string file = "./temp_"+to_string(j)+to_string(chrono::steady_clock::now().time_since_epoch().count() + rand())+".bin";
    m.unlock();
    out.open(file, ios::binary);
    if(!out.is_open()){
        cout << "Write Error!" << endl;
    }
    else{
        sort(arr, arr + n);
        out.write((char*)arr, n*sizeof(int32_t));
        q.push(file);
        out.close();
    }
}

void readIt(ifstream &fin, unsigned long n, unsigned long filesize, mutex &m, int j, SafeQueue &q){
    q.set_me_working(j, true);
    unsigned long a;
    unsigned long rest;
    unsigned long size; 
    while(true){
        m.lock();
        if (fin.peek() == EOF) {
            m.unlock();
            break;
        }
        a = fin.tellg();
        rest = (filesize - a)/sizeof(int32_t);
        size = min(rest, n);
        int32_t* nums = new int32_t[size];
        fin.read((char*)nums, size*sizeof(int32_t));
        m.unlock();
        start_sort(nums, size, j, q, m);
        delete[] nums;
    }    
    q.set_me_working(j, false);
}

void mergeIt(SafeQueue &q, int i, mutex &m){
    string file1, file2;
    while(true){
try{
        bool go = q.wait_pop(file1, file2);
        if(!go) break;
        q.set_me_working(i, true);
        ifstream fin1(file1, ios::binary);
        ifstream fin2(file2, ios::binary);
        m.lock();
        string new_file = "./"+to_string(i)+"temp_"+to_string(chrono::steady_clock::now().time_since_epoch().count()+rand())+".bin";
        m.unlock();
        int32_t l = 0, r = 0;
        fin2.read((char*)&r, sizeof(int32_t));
        fin1.read((char*)&l, sizeof(int32_t));
        ofstream out(new_file, ios::binary);
        while(true){
            if(fin1.peek() == EOF){
                out.write((char*)&l, sizeof(int32_t));
                while(fin2.peek() != EOF){
                    out.write((char*)&r, sizeof(int32_t));
                    fin2.read((char*)&r, sizeof(int32_t)) ;
                }
                    out.write((char*)&l, sizeof(int32_t));
                break;
            }
            if(fin2.peek() == EOF){
                out.write((char*)&r, sizeof(int32_t)) ;
                while(fin1.peek() != EOF){
                    out.write((char*)&l, sizeof(int32_t));
                    fin1.read((char*)&l, sizeof(int32_t)) ;
                }
                    out.write((char*)&l, sizeof(int32_t));
                break;
            }
            if(l < r){
                out.write((char*)&l, sizeof(int32_t));
                fin1.read((char*)&l, sizeof(int32_t)) ;
            }
            else{
                out.write((char*)&r, sizeof(int32_t));
                fin2.read((char*)&r, sizeof(int32_t)) ;
            }
        }
        fin1.close();
        fin2.close();
        remove(file1.c_str());
        remove(file2.c_str());
        out.close();
        q.push(new_file);
        q.set_me_working(i, false);
    }
catch (const char* str){
    cout << "Error" << endl;
    cout << str << endl;
}

}
    q.set_me_working(i, false);
}

void check(string file){
    cout << "Check result" << endl;
    ifstream fin(file, ios::binary);
    int32_t a, b;
    fin.seekg(0, fin.end);
    unsigned long filesize = fin.tellg();
    cout << filesize/sizeof(int32_t) << endl;
    fin.seekg(0, fin.beg);
    fin.read((char*)&a, sizeof(int32_t)) ;
    fin.read((char*)&b, sizeof(int32_t)) ;
    while(true){
        if (a > b){
            cout << "Sort_error" << endl;
            break;
        }
        if(fin.peek() != EOF){
            cout << "Sort_succ." << endl;
            break;
        }
        fin.read((char*)&a, sizeof(int32_t)) ;
        if(fin.peek() != EOF)
            fin.read((char*)&b, sizeof(int32_t)) ;
        else 
            break;
    }
}

int main(){
    ifstream fin("./test_file.bin");
    if(!fin.is_open()){
        cout << "Read Error!" << endl;
    }
    else{
        mutex m;
        int size_of_threads = thread::hardware_concurrency();
        SafeQueue q_instant(size_of_threads);
        vector<thread> threads;
        fin.seekg(0, fin.end);
        unsigned long filesize = fin.tellg();
        cout << filesize/sizeof(int32_t) << endl;
        fin.seekg(0, fin.beg);
        unsigned long n = 1000000; // число считываем чисел в потоком в начале
        for (int i = 0; i < size_of_threads; ++i){
           threads.emplace_back(thread(readIt, ref(fin), n, filesize, ref(m), i, ref(q_instant)));
        }
        for (int i = 0; i < size_of_threads; ++i)
            threads[i].join();
        fin.close();
        threads.clear();
        for(int i = 0; i < size_of_threads; ++i){
            threads.emplace_back(thread(mergeIt, ref(q_instant), i, ref(m)));
        }
        for(int i = 0; i < size_of_threads; ++i){
            threads[i].join();
        }
        threads.clear();
        check(q_instant.just_pop());
    }
}
