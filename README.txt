# bankclientserver
Client/Server Project for a bank system
The bank server holds information for a given client.
The server allows the user to do a number of things to get started
1. The user can Sign Up
2. The user can Sign In
3. The user can quit from the program and disconnect from the server
The client is allowed to choose any of these three options then based on the action chosen the server allows for new options
-If they choose sign up the server ask for a new unique username and password and if they do put a unique username and password
the server asks for the first and last name. 
-If they choose to sign in the client must sign in with an exisiting username and password combination. If they do then are signed in.
-In both siging in and signing up the user can type Quit into the Username area to quit the program and connection if they decide they 
do not want to sign up or sign in.
-For both as well they can try as many times as they want to sign up/sign in until they get it properly.
-Third option of quitting they are disconnected from the server.
After the first three options if they sign in/sign up they have 7 new options
1.Print the account information
2.Create a new checking or savings account
3.Deposit money into an account
4.Withdraw money from an account
5.Transfer money between accounts
6.Remove one of their accounts
7.Quit from the server
-Printing accont information just prints any monetary account info they have
-Creating a new account, creates a new account if they have space, a user can only have 4 total accounts, 1 savings, 3 checkings
so as long as they have less than 3 checkings they can create a new checking account, and as long as they dont have a savings account yet 
they can create one
-Deposits money into a specific account so long as they have one to deposit one into and they choose a valid account 
-Withdraws money into a specific account so long as they have one to withdraw from and they choose a valid account and the ammount they 
choose to withdraw does not overdraft their account
-Transfers money from one account to another so long as they have 2 or more accounts, each account they choose is valid, and
the amount they choose to transfer is not going to overdraft the account
-Removes an account and allows them to either transfer the money from the removing account to another or withdraw all to them so long
as they have an account to remove, and if they choose to transfer the account they choose is valid.
-Quit from the server disconnects them from the server and end the client program.

The server handles the account information and all changes and stores the User info in a data structure called UserData and puts
those structs in a map of all the user info structs and when appropriate puts them into a cache so that they can be accessed quickly
if they need to be. Server also handles the username and password storage as they are stored in a map of usernames and passwords.
Server writes both all username/password combos into a file as well as the map of all the user data into a separate file

The client handles the collection of the information that is entered by the user and sends that to the server. It also reads the 
responses from the server and tells the user what the server says to inform them on how their decisions fared.

In client.cpp be sure to add your own hostname for the client

lrucache.cpp is a test of the cache.
