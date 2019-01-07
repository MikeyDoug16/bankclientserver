#include <sys/types.h>
#include <signal.h>
#include <sys/uio.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <string.h>
#include <iostream>
#include "lrucache.h"
#include <fstream>
using namespace std;
#define PORT 8306
#define MAX_CLIENTS 40
string key= "HDsKdAk";

/*writes the given message to the output and then returns with an -1 on exit to show that there was an error*/
void error(char * message){
	cout<<message<<endl;
	exit(-1);
}

/*Encrypts the given string base and returns result. Uses the string key global variable "HDsKdAk" and goes thru each character in the base
string and exclusive ors them with the given letter in the key determined by moduloing the current index of the base by the size of the key*/
string encrypt(string base){
	string result(base);
	int sz = base.size() , sz2 = key.size();
	for(int i=0; i <sz; ++i)
		result[i] ^= key[i%sz2];
	return result;
}

/*Exits the program to signify a timeout of the server*/
void time_out(int arg)
{
	(void)arg;
    exit(0);
}

/*Writes every user from the map of users to the file USERINFO.database
goes thru each pair in the map and getting all the data from UserData in it.second, the four main strings (user, pass, fname [first name] 
lname [last name] ) are encrypted before being written to the file the num accounts, accountids, money and type of account are not encrytped
before being written to the file, counts how many accounts have been send and checks against num_accounts so no extra accounts are sent*/
void write_user_info(map<string, UserData> map_of_user_info){
	ofstream out("USERINFO.database", ios::out | ios::trunc);
	string user,pass,lname,fname,accType,numacc,mon,accid;
	int num_accounts, money, accountid,count=0;
	for(auto it: map_of_user_info){
		count =0;
		user = it.second.SignUpId;
		out<<encrypt(user)<<endl;
		pass = it.second.password;
		out<<encrypt(pass)<<endl;
		fname = it.second.fname;
		out<<encrypt(fname)<<endl;
		lname = it.second.lname;
		out<<encrypt(lname)<<endl;
		num_accounts = it.second.num_accounts;
		numacc = to_string(num_accounts);
		out<<numacc<<endl;
		for(int i=0; i<MAX_NUM_ACCOUNTS; ++i){
			if(it.second.accounts[i].money != -1 && num_accounts > count && it.second.accounts[i].accType != ""){
				++count;
				accountid = it.second.accounts[i].accountid;
				accid = to_string(accountid);
				out<<accid<<endl;
				money = it.second.accounts[i].money;
				mon = to_string(money);
				out<<mon<<endl;
				accType = it.second.accounts[i].accType;
				out<<accType<<endl;
			}
		}
	}
	out.close();
}

/*Sets the timeout of the server to the number of seconds passed in. Sets the itimerval struct to 0 and then sets the time
also sets the SIGALRM signal to have a handler which is time_out*/
void set_time_out(int seconds)
{
    struct itimerval value;// = {0};
	memset(&value,0, sizeof(value));
    value.it_value.tv_sec = seconds;
    setitimer(ITIMER_REAL, &value, NULL);
	signal(SIGALRM, time_out);
}

/*creates the socket for the server, sets the sockopt so that we reuse the address and sets the 
sockaddr to the appropriate settings as well as the port number*/
void create_socket(int * server_fd, struct sockaddr_in * addr, int opt){
	*server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(*server_fd == 0)
		error((char * )"Socket creation error");
	if(setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
		error((char * )"Setsockopt failure");
	addr->sin_family = AF_INET;
	addr->sin_addr.s_addr = INADDR_ANY;
	addr->sin_port = htons(PORT);
}

/*Does the bind and listen stage of the server creation proces setting the backlog of listen to 10*/
void bind_listen(int * server_fd, struct sockaddr_in addr){
	if (bind(*server_fd, (struct sockaddr *)&addr,sizeof(addr))<0)  
        error((char *)"bind failed"); 
    if (listen(*server_fd, 10) < 0) 
        error((char * )"listen failed"); 
}

/*function used often throughout the program to write buf to the client. First writes the size of the transmission to the client so that the client
knows how much to read (htonl and first write). Then it writes the real message to the client. Both writes are error checked in the case they do not
go through properly */
void write_to(int fd, char * buf){
	int size = strlen(buf),temp,n;
	temp = htonl(size);
	if((n=write(fd, (char *)&temp, sizeof(temp)))<0)
		error((char *)"Sending size error");
	if((n=write(fd, buf,size))<0)
		error((char *)"Sending message to client error");
}

/*function used often throughout the program to read from the client. Reads the size of the real transmission from the client so that it knows exactly how much
it needs to read for the real write from the client (ntohl and first read). Both reads are error checked in case they do not work properly. 
The buf is then cut off with the null terminator*/
void read_from(int fd, char * buf){
	int n, temp;
	if((n=read(fd,(char *)&temp, sizeof(temp)))<0)
		error((char *)"Reading size error");
	temp = ntohl(temp);
	if((n=read(fd,buf,temp))<0)
		error((char *)"Reading message error");
	buf[n] = '\0';
}

/*Writes he initial banking menu to the client by setting the char pointer to the menu string, copying it to a char array and writing it to the client
it then reads the response of the client, and turning it to an integer and then returning the integer*/
int initial_menu_write(int fd){
	char buf[256];
	char * menu = (char *)"Welcome to the Banking Inc Bank System!\nEnter the number of the choice you would like to do\n1.Sign up.\n2.Sign in.\n3.Quit.\n";
	write_to(fd, menu);
	read_from(fd,buf);	
	int input = atoi(buf);
	return input;
}

/*Function to end the client connection, writes to them signifying that the conneciton is closing and then writes it to them, finishing
it by closing the client*/
void end_client(int client,char * buf){
	char * response = (char *)"Quitting from the Banking Inc Banking System. Thank you for choosing Bank Inc! Good Bye\n";
	write_to(client, response);
	close(client);
	(void) buf;
}

/*reads a two time response from the client, reading two different answers (used for getting boh first and last name, both user and pass)*/
void read_response(int client, char * buf2, char * buf3){
	read_from(client, buf2);
	read_from(client, buf3);
}

/*Writes the user name and password given to the username/password file database. It encrypts both of them before writing them to the file for security*/
void write_to_userpass_file(string user, string password){
	string user_temp = encrypt(user);
	string pass_temp = encrypt(password);
	ofstream out("UsernamePassword.database", ios::out | ios::app);
	out<<user_temp<<endl;
	out<<pass_temp<<endl;
	out.close();
}

/*Creates a simple single account and makes it a savings or checking account based on the number of the account that is passed in
it then sets it to the proper spot in the UserData structure
called by create accounts and by the reading_user_info functions*/
void create_single_accounts(UserData * data, int num_of_account){
	Account acc;
	acc.accountid = num_of_account;
	acc.money = -1;
	if(num_of_account < 3)
		acc.accType = "Checking";
	else
		acc.accType = "Savings";
	data->accounts[num_of_account] = acc;
}

/*Create all the accounts of the UserData, called by the case 1: for when a user signs up, calls the create_single_accounts function above to create each one 
individually*/
void create_accounts(UserData * data){
	for(int i=0; i<MAX_NUM_ACCOUNTS; ++i){
		create_single_accounts(data, i);
	}
}

/*fills the UserData data structure with the given strings (user, pass lname, fname)
then once it puts those strings in their places it creates all the accounts needed for the UserData structure*/
void create_data(string user, string pass, string lname, string fname, UserData * data){
	data->SignUpId = user;
	data->password = pass;
	data->lname = lname;
	data->fname = fname;
	data->num_accounts = 0;	
	create_accounts(data);
}

/*Collect the account information of the user and send it to the client, if they have no accounts then tell them, if they do have accounts
write to the client how many reads it will need to (aka how many account infos it is sending), It accumulates the information in the string
buffer (getting the info from data.accounts[i] Account struct so long as the money is not == -1 [-1 = not a valid account] and then writes 
it using the c_str function to convert it to the type it needs to be to be sent to write_to*/
void print_accounts(UserData data, int client){
	char * response, size[2];
	string buffer = string();
	if(data.num_accounts == 0){
		response = (char *)"No accounts to print\n";
		write_to(client, response);
	}
	else{
		size[0] = data.num_accounts + '0', size[1] = '\0';
		write_to(client,size);
		response = (char *)"Here are your accounts!\n";
		write_to(client, response);
		for(int i=0; i<MAX_NUM_ACCOUNTS; ++i){
			if(data.accounts[i].money != -1 && data.accounts[i].accType != ""){
				buffer = string();
				buffer+="This is your " + data.accounts[i].accType + " account\n";
				buffer+="Accountid #" + to_string(data.accounts[i].accountid + 1) + "\n";
				buffer+="Money in the account: " + to_string(data.accounts[i].money) + "\n";
				write_to(client, (char *)buffer.c_str());
			}
		}
	}
	
}

/*Creates a user account so long as A. They dont have 4 accounts [max number of accounts allowed] B. They are making a checking account and have 2 or
fewer checking accounts [3 is max number of checking accounts allowed C. Making a savings account and have no savings account [only 1 savings account allowed total]
The server will tell the user if they do not meet this criteria as they send over their requests from the server (lines 243, 251, 255)
If they do meet the criteria of making a new account the server will do this in the switch case making a checking in case 1 and savings in case 2 setting the money
to 0 to indicate a valid account and increasing the num_accounts number by 1*/
void create_account(UserData * data, int client, char * buf){
	char * response;
	if(data->num_accounts == 4){
		response = (char *)"Sorry you already have the maximum number of accounts!\n";
		write_to(client, response);
	}
	else{
		response = (char *)"What type of account would you like to make?\n1.Checking\n2.Saving\n";
		write_to(client, response);
		read_from(client, buf);
		if(buf[0] == '1' && (data->accounts[0].money != -1) && (data->accounts[1].money != -1) && (data->accounts[2].money != -1)){
			response = (char *)"Sorry you already have the maximum number of checking accounts!\n";
			write_to(client, response);
		}
		else if(buf[0] == '2' && data->accounts[3].money != -1 ){
			response = (char *)"Sorry you already have the maximum number of saving accounts!\n";
			write_to(client, response);
		}
		else{
			switch(buf[0]){
				case '1':	
					for(int i=0; i<3; ++i){
						if(data->accounts[i].money == -1){
							data->accounts[i].money = 0;
							data->accounts[i].accType = "Checking";
							response = (char *)"New checking account created!\n";
							write_to(client, response);
							data->num_accounts++;
							break;
						}
					}
					break;
				case '2':
					data->accounts[3].money = 0;
					data->accounts[3].accType = "Savings";
					response = (char *)"New savings account created!\n";
					write_to(client, response);
					data->num_accounts++;
					break;
			}
		}
	}
}

/*Deposits money for a user into the account of their choosing so long as A. they have an account checking or savings and B.they choose an actually valid
account ie money != -1. The server will tell them if they fail to meet the criteria (lines 290, 299) otherwise it will do what the client requested and
deposit the money to the requested account number.*/
void deposit_money(UserData * data, int client, char * buf){
	char * response;
	int dep_amount=0;
	if(data->num_accounts == 0){
		response = (char *)"No accounts to deposit money into!\n";
		write_to(client, response);
	}
	else{
		response = (char *)"Which account would you like to deposit to (Enter Accountid 1-4)?\n";
		write_to(client, response);
		read_from(client, buf);
		int in = buf[0] - '0';
		if(data->accounts[in-1].money == -1){
			response = (char *)"Invalid account number!\n";
			write_to(client, response);
		}
		else{
			response = (char *)"Enter the money amount you want to deposit\n";
			write_to(client, response);
			read_from(client, buf);
			string s(buf);
			dep_amount = stoi(s,nullptr);
			data->accounts[in-1].money+=dep_amount;
			response = (char *)"Thank you for your deposit to our bank!\n";
			write_to(client, response);
		}
	}
}

/*Withdraws money from the users account as long as A. They have an account (num_accounts > 0) B. They choose a valid account ie money != -1 and 
C. The amount they choose to withdraw does not over draft their account. The server will tell them if they do not meet this criteria (lines 322 331 341)
The server reads the account number and the amount of money to withdraw from the client and then if the criteria is met it deducts the amount from
the specified account*/
void withdraw_money(UserData * data, int client, char * buf){
	char * response;
	int with_amount=0;
	if(data->num_accounts ==0){
		response = (char *)"No accounts to withdraw money from!\n";
		write_to(client, response);
	}
	else{
		response = (char *)"Which account would you like to withdraw from (Enter Accountid 1-4)?\n";
		write_to(client, response);
		read_from(client, buf);
		int in = buf[0] - '0';
		if(data->accounts[in-1].money == -1){
			response = (char *)"Invalid account number!\n";
			write_to(client, response);
		}
		else{
			response = (char *)"Enter the amount you want to withdraw\n";
			write_to(client, response);
			read_from(client, buf);
			string s(buf);
			with_amount = stoi(s, nullptr);
			if(data->accounts[in-1].money - with_amount < 0){
				response = (char *)"Error! Not enough money to withdraw that amount!\n";
				write_to(client, response);
			}
			else{
				response = (char *)"Thank you! Your withdrawal has been completed!\n";
				write_to(client, response);
				data->accounts[in-1].money-=with_amount;
			}
		}
	}
}

/*Transfers money from one account to another given A. They have at least two accounts (line 358) B. The first account they pick is valid (367)
C. The second account chosen isnt the same as the first account (line 377) D. the second account chosen is valid (line 381) E. the amount to transfer does not 
overdraft the first account 392. The server will tell the user/client if any of these fail (359, 368, 378, 382, 393).
The server will read from the user the first account number, check it, the second account number check that, then the amount of money to transfer and check that
if it finds all is well it will transfer the money from the first account to the second account*/
void transfer_money(UserData * data, int client, char * buf){
	char * response;
	if(data->num_accounts ==0 || data->num_accounts == 1){
		response = (char *)"Not enough accounts to transfer!\n";
		write_to(client, response);
	}
	else{
		response = (char *)"Which account would you like to transfer from (Enter Accountid 1-4)?\n";
		write_to(client, response);
		read_from(client, buf);
		int in = buf[0] -'0';
		if(data->accounts[in-1].money == -1){
			response= (char *)"Invalid account number!\n";
			write_to(client, response);
		}
		else{
			response = (char *)"Which account would you like to transfer to (Enter Accountid 1-4)?\n";
			write_to(client, response);
			read_from(client, buf);
			int in2 = buf[0] -'0';
			if(in2 == in){
				response = (char *)"Cannot transfer between the same account!\n";
				write_to(client, response);
			}
			else if(data->accounts[in2-1].money == -1){
				response = (char *)"Invalid account number!\n";
				write_to(client, response);
			}
			else{
				response = (char *)"Enter the amount you want to transfer\n";
				write_to(client, response);
				read_from(client, buf);
				string s(buf);
				int amount = stoi(s, nullptr);
				if(data->accounts[in-1].money - amount < 0){
					response = (char *)"Error! You do not have enough money to transfer this amount!\n";
					write_to(client, response);
				}
				else{
					response = (char *)"Transfer completed! Thank you for choosing Bank Inc!\n";
					write_to(client, response);
					data->accounts[in-1].money-=amount;
					data->accounts[in2-1].money+=amount;
				}
			}
		}
	}
}

/*Will remove an account given A. The user has accounts to remove (line 412) B. enters a valid account number (421) C. If they choose to transfer the accounts
money to another account that account must be a valid account and not be the same as the chosen account to be removed (line 438)
The server send the user/client any problems with the request (lines 413, 422, 439). The server reads the first account, checks it, asks for either
a second account to transfer funds or just withdraw all to the user, reads user answer, checks if the account (if option chosen) is valid, then if transfer is
chosen transfers the money or if the withdrawal is chosen does that instead*/
void remove_account(UserData * data,int client, char * buf){
	char * response;
	if(data->num_accounts == 0){
		response = (char *)"No accounts to remove!\n";
		write_to(client, response);
	}
	else{
		response = (char *)"Which account would you like to remove (Enter Accountid 1-4)?\n";
		write_to(client, response);
		read_from(client, buf);
		int in = buf[0] -'0';
		if(data->accounts[in-1].money == -1){
			response = (char *)"Invalid account number!\n";
			write_to(client, response);
		}
		else{
			if(data->num_accounts > 1){
				response = (char *)"Which account would you like the money in this account to go to (Enter Accountid 1-4, 0 to withdraw all)?\n";
				write_to(client, response);
				read_from(client, buf);
				int new_in = buf[0] - '0';
				if(new_in == 0){
					response = (char *)"Withdrawing money to you and removal of account is completed!\n";
					write_to(client, response);
					data->accounts[in-1].money = -1;
					data->num_accounts--;
				}
				else if(data->accounts[new_in-1].money == -1 || new_in == in){
					response = (char *)"Invalid account number!\n";
					write_to(client, response);
				}
				else{
					response = (char *)"Withdrawing money from your account and depositing it in newly specified account!\nRemoval of account is complete!\n";
					write_to(client, response);
					int cash = data->accounts[in-1].money;
					data->accounts[new_in-1].money+=cash;
					data->accounts[in-1].money = -1;
					data->num_accounts--;
				}
			}
			else{
				response = (char *)"Removed your only account. Withdrawing Money to you and removal of the account is completed!\n";
				write_to(client, response);
				data->accounts[in-1].money = -1;
				data->num_accounts--;
			}
		}
	}
}

/*Switches on the user input 1. prints accounts 2. creates an account 3. deposits money from account 4. withdraws money from account 5. transfers money between accounts
6. Remove an account default is quitting from the connection it updates the cache and the map of user info and ends connection*/
void switch_on_user_menu(int client, string user, UserData* data, LRUCache * cache, map<string, UserData> * map_of_user_info, int user_input){
	char buf[256];
	switch(user_input){
		case 1:
			print_accounts(*data, client);
			break;
		case 2:
			create_account(data,client,buf);
			break;
		case 3:
			deposit_money(data, client,buf);
			break;
		case 4:
			withdraw_money(data,client,buf);
			break;
		case 5:
			transfer_money(data,client, buf);
			break;
		case 6:
			remove_account(data, client,buf);
			break;
		default:
			(*map_of_user_info)[user] = *data;
			cache->set(user, *data);
			end_client(client, buf);
	}
}

/*Writes the main menu of the bank in a while loop until the user picks to quit. gets the user choice from the client and puts it into the user_input variable
send the input to the switch on user menu function to handle it*/
void user_work(int client, string user, UserData * data, LRUCache * cache, map<string, UserData> * map_of_user_info){
	char buf[512];
	int user_input=0;
	const char * response = "\nBanking Inc Main Menu!\nEnter the number for the action you would like to perform!\n1.Print Bank Account Balances\n2.Create New Bank Account\n3.Deposit Money\n4.Withdraw Money\n5.Transfer Money Between Accounts\n6.Remove an Account\n7.Quit\n";
	write_to(client, (char *)response);
	while(user_input != 7){	
		read_from(client, buf);
		user_input = buf[0] - '0';
		switch_on_user_menu(client, user, data, cache, map_of_user_info, user_input);
	}
	(void) user, (void) data, (void) cache, (void) map_of_user_info;
}

/*Case 1: Signing up, asks the client to enter a username and a password, if the username is "Quit" end the connection as the client wants to quit
if it isnt check if the user pass combo is in the userpass map, if it is not that then set correct to 1 to end the while loop, send that the pass word was set, 
put the user/password into the userpass map and ask the user to enter their first and last name, read both from the user, write the username and password to the 
userpass file database, create the UserData structure put it in the cache, put it in the overall user map as well and send it to user work. If the username/pass
is already taken tell the user.
Case 2: Signing in, get the username and password from the user, if the username is either not in the userpass map or if the password doesnt match
it denies the client and tells them. If they enter a correct username and password, it checks if they are in the cache, if they are, it updates the cache and 
gets their UserData from the cache. If it is not in the cache it gets the UserData from the map of user info, puts it in the cache then goes to user work
Case 3: This is to quit, it ends the client connection with the end_client function
After switch: updates the time out time for the server to 20 seconds and writes the map of user info to the user database */
void switch_on_input(int menu_input, int client, map<string, string> * userpass_map, LRUCache* cache, map<string, UserData > * map_of_user_info){
	char buf[512], * response, buf2[USERNAME_MAX],buf3[PASS_MAX], frname[UNAME_MAX], laname[UNAME_MAX];
	int correct=0;
	string user,pass,lname,fname;
	UserData data;
	switch(menu_input){
		case 1:
			response = (char *)"Please enter a New Username(up to 19 total characters) and Password(up to 14 total characters)\nUsername cannot contain spaces\nType \"Quit\" to cancel the sign up\n";
			write_to(client, response);
			while(correct == 0){
				read_response(client,buf2,buf3);
				if(strcmp(buf2, "Quit") ==0){
					end_client(client,buf);
					break;
				}
				user = string(buf2);
				pass = string(buf3);
				auto it = userpass_map->find(user);
				if(it == userpass_map->end()){
					correct = 1;
					(*userpass_map)[user]=pass;
					response = (char *)"UserName and Password successfully set!\n";
					write_to(client, response);
					response = (char *)"Please enter your first and last name (both under 20 characters)\n";
					write_to(client, response);
					read_response(client, frname, laname);
					lname = string(laname);
					fname = string(frname);
					write_to_userpass_file(user, pass);
					create_data(user,pass,lname, fname, &data);
					cache->set(user, data);
					(*map_of_user_info)[user] = data;
					user_work(client, user, &data, cache, map_of_user_info);
					
				}
				else{
					response = (char*)"UserName/Password already taken, please try again\n";
					write_to(client, response);
				}
			}
			break;
		case 2:
			response = (char *)"Please enter your username and password.\nIf you do not have a username/password, type Quit in the username, restart the program, and sign up! Thanks!\n";
			write_to(client, response);
			while(true){
				read_response(client,buf2,buf3);
				if(strcmp(buf2,"Quit") == 0){
					end_client(client, buf);
					break;
				}
				else{
					user = string(buf2);
					pass = string(buf3);
					if(userpass_map->find(user) != userpass_map->end() && pass.compare((*userpass_map)[user]) == 0){
						response = (char *)"Successfully Signed in!\n";
						write_to(client, response);
						if(cache->in_cache(user) == false){
							auto it = map_of_user_info->find(user);
							cache->set(user, it->second);
							UserData data = it->second;
							user_work(client,user, &data, cache, map_of_user_info);
						}
						else{
							UserData * dataptr = cache->get(user);
							user_work(client, user, dataptr, cache, map_of_user_info); 
						}
						break;
					}
					else{
						response = (char *)"Username/Password combination are incorrect please try again\n";
						write_to(client, response);
					}
				}
			}
			break;
		default:
			end_client(client, buf);
			break;	
	}
	write_user_info(*map_of_user_info);
	set_time_out(20);//, *map_of_user_info);
}

/*Read the usernames and passwords from the database, unencrypts them then puts them into the map of the usernames and passwords*/
void read_usernames_passwords(map<string, string> * map_of_users){
	string user,pass;
	ifstream in("UsernamePassword.database", ios::in);
	if(in.is_open()){
		while(getline(in,user) && getline(in, pass)){
			user = encrypt(user);
			pass = encrypt(pass);
			(*map_of_users)[user]=pass;
		}
	}
	in.close();
}

/*Resets the data structure UserData to the defaults*/
void reset_data(UserData * data){
	data->SignUpId = "";
	data->password = "";
	data->fname = "";
	data->lname = "";
	for(int i=0; i<MAX_NUM_ACCOUNTS; ++i){
		data->accounts[i].money = -1;
		data->accounts[i].accountid = i;
		if(i<3)
			data->accounts[i].accType = "Checking";
		else
			data->accounts[i].accType = "Savings";
	}	

}

/*Resets the data struct UserData then puts the rest of the parameters into the struct*/
void set_user_data(string user, string pass, string fname, string lname, int num_accounts, UserData * data){
	reset_data(data);
	data->SignUpId = user;
	data->password = pass;
	data->fname = fname;
	data->lname = lname;
	data->num_accounts = num_accounts;
}

/*Sets the parameters money accountid and accType into the Account struct*/
void set_account(int money, int accountid, string accType, Account * acc){
	acc->accountid = accountid;
	acc->money = money;
	acc->accType = accType;
}

/* Reads the user info from the USERINFO file database, it reads the username, password, first name and last name then decrypts them, reads the num of accounts
puts them into the UserData struct and then begins to read the Accounts from the file for the UserData struct. reads the money accountid and the accType for each
account in the file for the given user, for all the accounts not read from the file it creates default invalid accounts for the others, puts the accounts
into the map of user info*/
void read_user_info(map<string, UserData> * map_of_user_info){
	string user,pass,lname,fname, accType,num_acc, mon, accid;	
	int num_accounts, accountid, money;
	bool arr[MAX_NUM_ACCOUNTS] = {false};
	ifstream in("USERINFO.database", ios::in);
	if(in.is_open()){
		while(getline(in,user)){
			user = encrypt(user);
			UserData data;
			arr[0] = false, arr[1] = false, arr[2] = false, arr[3] = false;
			getline(in, pass);
			pass = encrypt(pass);
			getline(in, fname);
			fname = encrypt(fname);
			getline(in, lname);
			lname = encrypt(lname);
			getline(in, num_acc);
			num_accounts = stoi(num_acc, nullptr);
			set_user_data(user,pass,fname,lname,num_accounts, &data);
			for(int i=0; i< num_accounts; ++i){
				Account acc;
				getline(in,accid);
				accountid = stoi(accid, nullptr);
				getline(in, mon);
				money = stoi(mon, nullptr);
				getline(in, accType);
				set_account(money, accountid, accType, &acc);
				data.accounts[accountid] = acc;
				arr[accountid] = true;
			}
			for(int i=0; i<MAX_NUM_ACCOUNTS; ++i){
				if(arr[i] == false){
					create_single_accounts(&data, i);
				}
			}
			(*map_of_user_info)[user] = data;	
		}
	}
	in.close();
}


/*Creates the server, cache, map of users, map of usernames/passwords and reads the usernames and passwords as well as the user info from their respective files
then loops from 0 to MAX_CLIENTS(40) and accepts them, sets timeout for 1 hour and begins client comms
EVENTUALLY will have threading and allow for concurrency*/
int main(){
	int server_fd,menu_input;
	struct sockaddr_in addr;
	int opt =1,	addrlen = sizeof(addr);
	int fd_array[MAX_CLIENTS]= {0};
	map<string, string> map_of_users;
	LRUCache cache(15);
	map<string, UserData > map_of_user_info;
	read_usernames_passwords(&map_of_users);
	read_user_info(&map_of_user_info);
	create_socket(&server_fd, &addr, opt);
	bind_listen(&server_fd, addr);
	set_time_out(20);
	for(int i =0; i< MAX_CLIENTS; ++i)
	{
		if((fd_array[i] = accept(server_fd,(struct sockaddr *)&addr, (socklen_t *)&addrlen))< 0)
			error((char *)"Accept error");
		else
		{
			set_time_out(3600);
			cout<<"Accepted Client for 1 hour!"<<endl;
			menu_input = initial_menu_write(fd_array[i]);	
			switch_on_input(menu_input, fd_array[i], &map_of_users, &cache, &map_of_user_info);
		}
	}
	close(server_fd);
	return 0;
}

