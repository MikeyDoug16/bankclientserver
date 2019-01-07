#include <iostream>
#include <map>
#include <list>
#include <utility>
#include <exception>
#include <bits/stdc++.h>
#include <string>
using namespace std;
#define PASS_MAX 14
#define USERNAME_MAX 19 
#define UNAME_MAX 20
#define MAX_NUM_ACCOUNTS 4
#define ACCTYPE_MAX 32


struct Account{
	int accountid;
	int money;
	string accType;
};

struct UserData{
	string SignUpId;
	string password;
	string fname;
	string lname;
	int num_accounts;
	Account accounts[MAX_NUM_ACCOUNTS];
};


struct CacheEmptyException:public exception{
	const char * what() const throw() {
		return "Cache Exception, Using get() on an empty cache\n";
	}
};

struct CacheEntryNotFoundException:public exception{
	const char * what() const throw(){
		return "Entry Not Found in the Cache Exception, get() could not find the key you were looking for\n";
	}
};
	
class LRUCache{
public:
	typedef list<pair<string, UserData >>::iterator list_it;
	LRUCache(int max_size)
	:cache_size(max_size){}
	void set(string key, UserData value){
		auto it = cache_map.find(key);
		if(it == cache_map.end()){
			if(cache_map.size() +1 > cache_size){
				auto last = cache_list.crbegin();
				cache_map.erase(last->first);
				cache_list.pop_back();
			}
			cache_list.push_front(std::make_pair(key,value));
			cache_map[key] = cache_list.begin();
		}
		else{
			it->second->second = value;
			cache_list.splice(cache_list.cbegin(), cache_list, it->second);
		}	
	}
	UserData * get(string key){
		try{
			if(cache_map.size() == 0){
				throw CacheEmptyException();
				return nullptr;
			}
			auto it = cache_map.find(key);
			if(it == cache_map.end()){
				throw CacheEntryNotFoundException();
				return nullptr;
			}
			cache_list.splice(cache_list.cbegin(), cache_list, it->second);
			return &(it->second->second);
		}
		catch(CacheEmptyException & e){
			//cout<<"Caught "<<e.what();
			return nullptr;
		}		
		catch(CacheEntryNotFoundException & e){
			//cout<<"Caught "<<e.what();
			return nullptr;
		}
	}
	bool in_cache(string key){
		return cache_map.find(key) != cache_map.end();
	}
	void print_cache(){
		list_it it = cache_list.begin();
		cout<<"CACHE"<<endl;
		for(; it!=cache_list.end(); ++it){
			cout<<"KEY: "<<it->first<< "\nVALUE: "<<it->second.SignUpId<<"  "<<it->second.num_accounts<<endl; 
		}
	}
private:
	map<string, list_it> cache_map;
	list<pair<string , UserData >> cache_list;
	unsigned int cache_size;
};

