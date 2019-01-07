#include <iostream>
#include "lrucache.h"
#include <string.h>
using namespace std;

void make_rand_str(int num, char * buf){
	int i;
	for(i=0; i<10; ++i){
		buf[i] = num + 'A';
	}
	buf[i] = '\0';
}

void tester(UserData * dataArr){
	char buf[11];
	for(int i=0; i<40; ++i){
		make_rand_str(i,buf);
		UserData data;
		data.SignUpId = string(buf);
		data.password = string(buf);
		data.fname= string(buf);
		data.lname = string(buf);
		data.num_accounts =0;
		for(int j=0; j<4; ++j){
        	Account a;
        	a.accountid = -1;
        	a.money= 0;
        	if(j<3)
            	a.accType= string("Checking");
        	else
            	a.accType=string("Saving");
        	data.accounts[j] = a;
    	}	
		dataArr[i] = data;
	}
}

void get_exception_test(){
	LRUCache cache (3);
	UserData * ptr = cache.get("Hello");
	if(ptr == nullptr)
		cout<<"We got nothing in the cache"<<endl;
	UserData data;
	cache.set("Bye", data);
	ptr = cache.get("ByeBye");
	if(ptr == nullptr)
		cout<<"We got nothing in the cache"<<endl;
}


int main(){
	LRUCache cache(15);
	UserData dataArr[40];
	tester(dataArr);
	for(int i=0; i<15; ++i){
		cache.set(dataArr[i*2].SignUpId, dataArr[i*2]);
	}
	cache.print_cache();
	UserData * dataptr = cache.get(dataArr[4].SignUpId);
	cout<<"GOT FROM CACHE GET "<<dataptr->SignUpId<<"  "<<dataptr->fname<< "    "<<dataptr->accounts[0].accType <<endl;
	cout<<endl;
	cache.print_cache();
	
	for(int i=1; i<=9; i+=2){
		cache.set(dataArr[i].SignUpId, dataArr[i]);
	}
	cout<<endl;
	cache.print_cache();
	cout<<endl;
	get_exception_test();
	if(cache.in_cache("EEEEEEEEEE") == true)
		cout<<"true"<<endl;
	if(cache.in_cache("ERERERERER") == false)
		cout<<"false"<<endl;
	string s = "Savings", res(s), key = "HDsKdAk";
	int j = s.size();
	for(int i=0; i<j; ++i){
		res[i] ^= key[i%key.size()];
		cout<<res[i];
	}
	cout<<endl;
	return 0;
	
}

