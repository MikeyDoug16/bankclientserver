#include <iostream>
#include <sys/uio.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>
#include <string.h>
#include <strings.h>
#include "lrucache.h"

using namespace std;

/*Error function takes the message and prints it to the console and then exits with -1 status to signal an error*/
void error(const char * message){
	cout<<message<<endl;
	exit(-1);
}

/*Creates and sets the sockaddr_in struct to the proper settings, gets the host name and puts it into the hostent strcut and then sets the port*/
struct sockaddr_in make_server_addr(char *host, short port){
    struct sockaddr_in addr;
    bzero(&addr, sizeof addr);
    struct hostent *hp = gethostbyname(host);
    if ( hp == 0 )
        error(host);
    addr.sin_family = AF_INET;
    bcopy(hp->h_addr_list[0], &addr.sin_addr, hp->h_length);
    addr.sin_port = htons(port);
    return addr;
}

/*makes the socket and then connects to the server, error checking the creation of the socket and the connection*/
int connect_socket(char *host, short port){
    int status;
    struct sockaddr_in addr = make_server_addr(host, port);
    int s = socket(AF_INET, SOCK_STREAM, 0);
    if ( s == -1 )
        error("socket()");
    status = connect(s, (struct sockaddr*)&addr, sizeof addr);
    if ( status < 0 )
        error("connect refused");
    return s;
}

/*reads from the server, first reads from the server to get the size of the real message then reads the real message
and only reads the size of the specified amount from the first read, both reads error checked, then buf is null terminated*/
void read_from(int client, char * buf){
	int n,temp;
	if((n = read(client, (char *)&temp, sizeof(temp)))<0)
		error("reading size error");
	temp = ntohl(temp);
	if((n = read(client, buf, temp))<0)
		error("reading message from server error");
	buf[n] = '\0';
}

/*A template function to get information from the client from the terminal, ignores the newline in the cin extractor and clears the extractor as well
then returns what was gotten from the user*/
template<class T>
T get_client_info(T buf){
	cin>>buf;
	cin.ignore();
	cin.clear();
	return buf;
}

/*Gets the opening menu from the server, and prints it, then in a while loop gets the user response for the menu from first the extractor then stringstreams
it to the input variable, so that if invalid input is entered it is handled*/
int get_menu(int client){
	char buf[512];
	int input=0;
	string integ;
	read_from(client, buf);
	cout<<buf;
	while(input != 1 && input !=2 && input != 3)
	{
		cout<<"Your response: ";
		integ = get_client_info<string>(integ);
		stringstream(integ) >> input;
		stringstream(string());
	}
	return input;
}

/*Writes to the server, first writes the size of the real message to the server so the server knows how much to read for the message then
writes the real message, error checking both writes*/
void write_to(int client, char * buf){
	int size, temp,n;
	size= strlen(buf);
	temp = htonl(size);
	if((n = write(client, (char *)&temp, sizeof(temp)))<0)
        error("Writing size error");
    if((n = write(client, buf, size))<0)
		error("writing buf error");
}

/*sends the parameter menu to the server, puts the menu parameter into a char array and sends it*/
void send_menu_choice(int menu, int client){
	char choice = menu + '0';
	char buf[2];
	buf[0] = choice, buf[1] = '\0';
	write_to(client, buf);
}

/*Gets the username and the password from the user then writes them both to the server
uses the get_client_info function with the char pointer type to get the info from the user*/
void username_password_work(int client, char * buf, char * user, char * pass){
	cout<<buf<<"Username: ";
	get_client_info<char *>(user);
	cout<<"Password: ";
	get_client_info<char *>(pass);
	write_to(client, user);
	write_to(client, pass);
}

/*Printing the accounts of the user: reads the initial buffer from the server
if it says no accounts then it prints it and that is all, if it has accounts to send the first buf has the number of accounts to read from the server
then it reads "Here are your accounts" then loops to read and print the accounts from the server */
void read_and_print_menu(int client, char * buf){
	char init_buf[128],acc[256]; 
	read_from(client, buf);
	if(strcmp(buf, "No accounts to print\n") == 0)
		cout<<buf;
	else{
		read_from(client, init_buf);
		cout<<"\n"<<init_buf<< "\n";
		for(int i=0; i<buf[0] - '0'; ++i){
			read_from(client, acc);
			cout<<acc<<endl;
		}
	}	
}

/*Creating an account for the user, reads from the user, either they have max accounts which ends the program or they have space so the server
asks what type of account to create, the user responds with either 1 or 2 again using stringstreams to keep poor input out. sends the user response
to the server, reads the server response and prints it*/
void create_account(int client, char * buf){
	int input=0;
	string integ;
	read_from(client, buf);
	cout<<buf;
	if(strcmp(buf,"What type of account would you like to make?\n1.Checking\n2.Saving\n") == 0){
		while(input != 1 && input!=2){
			cout<<"Your response: ";
			integ = get_client_info<string>(integ);
			stringstream(integ) >> input;
			stringstream(string());
		}
		send_menu_choice(input, client);
		read_from(client, buf);
		cout<<buf;
	}
}

/*sends the money amount in dep, first to strings the int into a string and then writes it to the server */
void send_money_choice(int dep, int client){
	string res = to_string(dep);
	write_to(client, (char *)res.c_str());
}

/*Deposits money to the users account at the specified account with a specific amount of money specified by the user
reads from the server either they have no accounts or if they do it asks what accountid, the user then inputs and responds (lines 177-183)
reads the server response, if it is an invalid account number it ends, if it isnt then the user inputs the money amount to deposit and sends it
then reads the user response*/
void deposit_money(int client){
	int input=0, dep = -1;
	string integ;
	char buf[256];
	read_from(client, buf);
	cout<<buf;
	if(strcmp(buf, "Which account would you like to deposit to (Enter Accountid 1-4)?\n")==0){
		while(input < 1 || input > 4){
			cout<<"Your response: ";
			integ = get_client_info<string>(integ);
			stringstream(integ) >> input;
			stringstream(string());	
		}
		send_menu_choice(input, client);
		read_from(client, buf);
		cout<<buf;
		if(strcmp(buf, "Enter the money amount you want to deposit\n") == 0){
			while(dep < 1){
				cout<<"Your money amount: ";
				integ = get_client_info<string>(integ);
				stringstream(integ) >> dep;
				stringstream(string());
			}	
			send_money_choice(dep, client);
			read_from(client, buf);
			cout<<buf;
		}
	}
}

/*Withdrawing money from a specified account and a specified amount. Reads from the server, prints it, and if they have no account it ends, if they have one
the server asks which account to withdraw from, they respond with get client info and send menu choice then the server responds and the client prints it 
if it is an invalid account it ends else the user was asked to enter the amount to withdraw which they then input and send, the server responds, which is where
the client reads, prints and ends the function*/
void withdraw_money(int client){
	char buf[256];
	int input=0, with = -1;
	string integ;
	read_from(client, buf);
	cout<<buf;
	if(strcmp(buf, "Which account would you like to withdraw from (Enter Accountid 1-4)?\n") ==0){
		while(input < 1 || input > 4){
			cout<<"Your response: ";
			integ = get_client_info<string>(integ);
			stringstream(integ) >>input;
			stringstream(string());
		}	
		send_menu_choice(input, client);
		read_from(client, buf);
		cout<<buf;
		if(strcmp(buf, "Enter the amount you want to withdraw\n") == 0){
			while(with < 1){
				cout<<"Your money amount: ";
				integ = get_client_info<string>(integ);
				stringstream(integ) >> with;
				stringstream(string());
			}	
			send_money_choice(with,client);
			read_from(client, buf);
			cout<<buf;
		}
	}
}

/*Transfering money from one account to another, account 1 specified by user, account 2 specified by user, money ammount specified by user
client reads, prints buf, checks if their request was valid, if buf was which account... they then can specify the account to transfer from
they send the response to server, server responds and the client reads and prints response. Either their account num sent was invalid or they were asked to
enter a second account. get second account, send it, read server response, print response. Either the second account num was invalid or they enter the 
amount to transfer, they send it server responds, client reads it, then prints response. It will print either success, amount to transfer is too much. */
void transfer_money(int client){
	char buf[256];
	int input=0, input2=0, transfer = -1;
	string integ;
	read_from(client, buf);
	cout<<buf;
	if(strcmp(buf, "Which account would you like to transfer from (Enter Accountid 1-4)?\n") == 0){
		while(input < 1 || input > 4){
			cout<<"Your response: ";
			integ = get_client_info<string>(integ);
			stringstream(integ) >> input;
			stringstream(string());
		}
		send_menu_choice(input, client);
		read_from(client, buf);
		cout<<buf;
		if(strcmp(buf, "Which account would you like to transfer to (Enter Accountid 1-4)?\n") == 0){
			while(input2 < 1 || input2 > 4){
				cout<<"Your response: ";
				integ = get_client_info<string>(integ);
				stringstream(integ) >> input2;
				stringstream(string());
			}
			send_menu_choice(input2, client);
			read_from(client, buf);
			cout<< buf;
			if(strcmp(buf, "Enter the amount you want to transfer\n") == 0){
				while(transfer < 1){
					cout<<"Enter amount: ";
					integ = get_client_info<string>(integ);
					stringstream(integ) >> transfer;
					stringstream(string());
				}
				send_money_choice(transfer,client);
				read_from(client, buf);
				cout<<buf;
			}
		}
	}
}

/*Removing an account: The user removes a specified account and can either withdraw all the money or transfer that money to a second account.
The client reads from the server and prints out the buf. If they have no accounts it ends, else they enter the account to remove. That is entered, sent
and then a response is read from the server. Either they entered an invalid account and it ends or they enter an acountid or to withdraw all funds
they send the entry and read the response from the server and print it.*/
void remove_account(int client){
	char buf[256];
	int input = 0;
	string integ;
	read_from(client, buf);
	cout<< buf;
	if(strcmp(buf, "Which account would you like to remove (Enter Accountid 1-4)?\n") == 0){
		while(input < 1 || input >4){
			cout<<"Your response: ";
			integ = get_client_info<string>(integ);
			stringstream(integ) >> input;
			stringstream(string());
		}
		send_menu_choice(input, client);
		read_from(client, buf);
		cout<<buf;
		input = -1;
		if(strcmp(buf, "Which account would you like the money in this account to go to (Enter Accountid 1-4, 0 to withdraw all)?\n") == 0){
			while(input < 0 || input > 4){
				cout<< "Your response: ";
				integ = get_client_info<string>(integ);
				stringstream(integ) >> input;
				if(input == 0 && (integ[0] != '0' || integ.size() > 1))
					input = -1;
				stringstream(string());
			}	
			send_menu_choice(input, client);
			read_from(client, buf);
			cout<<buf;
		}
	}
}

/*The user reads the banking menu from the server. User gets to enter their response and their response triggers a function
1. print their accounts 2. create a new account 3. deposit money into an account 4. withdraw money from an account 5. transfer money from one account to another
6. remove an account default. sets the outter while loop to end the user experience, reads the ending response from the server, prints it, closes client side of the 
connection*/
void client_work(int client, char * user){
	char menu_buf[512],buf[512];
	string integ;
	int input=0,finished = 0;
	read_from(client,menu_buf);
	while(finished == 0){
		input =0;
		cout<<menu_buf;
		while(input <=0 || input > 7){
			cout<<"Your response: ";
			integ = get_client_info<string>(integ);
			stringstream(integ)>>input;
			stringstream(string());
		}
		send_menu_choice(input, client);
		switch(input){
			case 1:
				read_and_print_menu(client, buf);
				break;
			case 2:
				create_account(client,buf);
				break;
			case 3:
				deposit_money(client);
				break;
			case 4:
				withdraw_money(client);
				break;
			case 5:
				transfer_money(client);
				break;
			case 6:
				remove_account(client);
				break;
			default:
				finished=1;
				read_from(client,buf);
				cout<<buf; 
				close(client);
			
		}
	}
	(void) user, (void) input;
}

/*Opening menu for the user to sign up sign in or quit
case 1: sign up. the user enters a username and password that is sent to the server, if the user name is quit the program quits, then
the client gets the confirmation of whether or not they entered a valid username and password. if they did, then they send their first and last name
to the server and begin the client work. if they did not enter a new username and password then they try again
case 2: Signing in. They enter a username and password, if the username is Quit then the client quits, else it sends it to the server
if they send a username and password that is valid then it goes on to the client work else they get to try to keep signing in.
default: Quitting, it reads the ending sign off from the server then closes the client connection */
void switch_on_menu(int menu, int client){
	int correct=0;
	char buf[512], user[USERNAME_MAX], pass[PASS_MAX],lname[UNAME_MAX], fname[UNAME_MAX], init_buf[512];
	switch(menu){
		case 1:
			read_from(client, init_buf);
			while(correct == 0){
				username_password_work(client, init_buf,user,pass);
				read_from(client,buf);
				cout<<buf;
				if(strcmp(user, "Quit") == 0)
					break;
				if(strcmp(buf, "UserName and Password successfully set!\n") == 0){
					correct = 1;
					read_from(client, buf);
					cout<<buf;
					cout<<"Enter First Name: ";
					get_client_info<char *>(lname);
					cout<<"Enter Last Name: ";
					get_client_info<char *>(fname);	
					write_to(client, lname);
					write_to(client,fname);	
					client_work(client,user);			
				}
			}
			close(client);
			break;
			
		case 2:
			read_from(client, init_buf);
			while(true){
				username_password_work(client, init_buf, user, pass);
				if(strcmp(user, "Quit")==0){
					read_from(client,buf);
					cout<<buf;
					break;
				}
				else{
					read_from(client, buf);
					cout<<buf;
					if(strcmp(buf,"Successfully Signed in!\n")  == 0){
						client_work(client,user);
						break;
					}
				}
			}
			close(client);
			break;
		default:
			read_from(client, buf);
			cout<<buf;
			close(client);
	}
}

/*Connects to the server, gets the opening menu action from the client to either sign up quit or sign in, they sends the choice to the server and finally
performs the proper actions based on the menu choice */
int main()
{
	const char * host = "";/*Put your own host for your own server here*/
	short port = 8306;
	int client = connect_socket((char *)host,port), menu;
	menu = get_menu(client);
	send_menu_choice(menu, client);
	switch_on_menu(menu,client);
	return 0;
}


