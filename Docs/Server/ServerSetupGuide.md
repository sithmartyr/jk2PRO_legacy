# Setting up a jk2PRO Server

Firstly, I recommend not having your Jedi Knight II server installed in 
Program Files/Program Files (x86) directory if you are using Windows.  

Create a directory within your server's GameData folder named jk2pro
Create a folder within your newly created jk2pro folder named vm
After you have compiled the game module, copy your jk2mpgame.qvm file 
over to jk2pro/vm/

NOTE: If you are using jk2mv, be sure to have the cvar mv_serverversion set to "1.02"

Use the server_template.cfg located in Docs/Server/ in the master branch of jk2pro's repository if 
you do not know how to configure the server configuration file.  

Once you have your server_template file configured the way you want it, you can then copy it 
over to your /jk2pro/ folder that you created previously.  Rename the file to server.cfg

Use a command like the one below to start your server:

#### Windows
	jk2mvded.exe +set dedicated 2 +set fs_homepath "." +set fs_game jk2pro +exec server.cfg

#### Linux
	./jk2mvded +set dedicated 2 +set fs_homepath "." +set fs_game jk2pro +exec server.cfg
	
If Linux yells at you with something like this 
	./jk2mvded: Permission denied

Then you will most likely need to give your binary file permission to be executed.  Run this command below:
	sudo chmod +x ./jk2mvded 
	
That's it! Hopefully after following this guide, you should have a running jk2PRO server!  Good luck & enjoy!