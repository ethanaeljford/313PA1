/*
	Original author of the starter code
    Tanzir Ahmed
    Department of Computer Science & Engineering
    Texas A&M University
    Date: 2/8/20
	
	Please include your Name, UIN, and the date below
	Name: Ethanael Joseph Ford
	UIN: 933009775
	Date: 09/23/2025
*/
#include "common.h"
#include "FIFORequestChannel.h"
#include <unistd.h>
#include <sys/wait.h>
#include <string> 
#include <fstream>
#include <iostream> // use f open to write the ECG's into a file for step 2. 

using namespace std; 
using std::string;




int main (int argc, char *argv[]) {
	int opt;
	int p = 1;
	double t = 0.0;
	int e = 1;
	int max = MAX_MESSAGE; // buffer capacity
	bool new_chan, inputE, inputT = false; // check if new chanel is needed

	int status; // used when killing server
	
	string filename = "";
	while ((opt = getopt(argc, argv, "p:t:e:f:m:c")) != -1) {
		switch (opt) {
			case 'p':
				p = atoi (optarg);
				break;
			case 't':
				t = atof (optarg);
				inputT = true;
				break;
			case 'e':
				e = atoi (optarg);
				inputE = true;
				break;
			case 'f':
				filename = optarg;
				break;
			case 'm': // add the buffer size flag 
				max = atoi (optarg);
				break;
			case 'c':
				new_chan = true;
				break;
		}
	}

	char* serverARGS[1] = {};

	pid_t server_id = fork();
    if (server_id == 0) {
		execvp("./server", serverARGS);
    }

	

    FIFORequestChannel chan("control", FIFORequestChannel::CLIENT_SIDE);


	FIFORequestChannel* newChan = &chan;

	if (new_chan) {
		char newName[MAX_MESSAGE];

		MESSAGE_TYPE m = NEWCHANNEL_MSG;
		chan.cwrite(&m, sizeof(MESSAGE_TYPE));
		chan.cread(newName, sizeof(newName));

		newChan = new FIFORequestChannel(newName, FIFORequestChannel::CLIENT_SIDE);

	}

	if (inputE && inputT && (filename == "")) { // GIVE SINGLE OUTPUT IF E AND T ARE GIVEN!
		double reply; // reply var
		
		char buf[MAX_MESSAGE]; // 256
		datamsg x(p, t, e);		
		memcpy(buf, &x, sizeof(datamsg));
		chan.cwrite(buf, sizeof(datamsg)); // question
		chan.cread(&reply, sizeof(double)); //answer
		cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl; // output
	}

	else { // Else write to file

		// Thousanda For (i < 1000) { 2 data msg per it - time ecg1, time ecg2 (decimals). USE i as timestamp (i*0.04). Read datamsg request into double time. 	run bimdc to check to see if it is the same file }
		// ()

		//chan.cwrite();
		
		// example data point request

		// Check if p != -1

		ofstream newFile("received/x1.csv"); // open file 
		//newFile.open("received/x1.csv"); // openn file once

		for (int i = 0; i < 1000; i++) { // Iterate for 1000 lines
			t = (i*.004); // Time is in .004 second increments
			double reply1, reply2; // Make two variables for ECGS
			e = 1;

			char buf[MAX_MESSAGE]; // 256
			datamsg x1(p, t, e);		
			memcpy(buf, &x1, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg)); // question
			chan.cread(&reply1, sizeof(double)); //answer
			// cout << "For person " << p << ", at time " << t << ", the value of ecg " << e << " is " << reply << endl;
			e = 2;
			datamsg x2(p, t, e);
			memcpy(buf, &x2, sizeof(datamsg));
			chan.cwrite(buf, sizeof(datamsg)); // question
			chan.cread(&reply2, sizeof(double)); //answer

			//cout << t << "," << reply1 << "," << reply2 << "\n";

			// write onto file called x1.csv
			newFile << t << "," << reply1 << "," << reply2 << "\n"; 		
			
		}
		newFile.close();
	}


    // sending a non-sense message, you need to change  
	// FOR LOOP - TAKE IN CHUNKS - WHEN REQUESTION A FILE SPECIFY HOW MANY BITES TO TAKE AND OFFSET -

	if (filename != "") {

		filemsg fm(0, 0);
		string fname = filename;
		
		int len = sizeof(filemsg) + (fname.size() + 1);
		char* buf2 = new char[len];
		memcpy(buf2, &fm, sizeof(filemsg));
		strcpy(buf2 + sizeof(filemsg), fname.c_str());
		chan.cwrite(buf2, len);  // I want the file length;

		__int64_t filesize = 0;
		chan.cread(&filesize, sizeof(__int64_t)); // Get the size of the new file
		__int64_t currSize = filesize;
		
		char * buf3 = new char[max]; // buffer 3 to write into the new file with custom buffer size

		//cout << filesize << "\n";

		filemsg* file_req = (filemsg*)buf2; // recast the pointer to reuse buffer 2 
		int smallSize = filesize % max; // use modulo to find the smaller buffer size for the last segment of the read. 
		int timesIterate = filesize / max; // Figure out how many times you need to iterate through the file

		if (smallSize != 0) { 
			timesIterate++; // If the buffer does not fully "fit" into the file, iterate an extra time
		}

		ofstream secondFile("received/" + fname); // read into the desired file

		for (int i = 0; i < timesIterate; i++) {
			file_req->offset = i * max; // Offset buy the amount you have already traversed. 
			if (currSize < max) { // Check how big you are going to read. 
				file_req->length = smallSize;
			} else {
				file_req->length = max; 
			}
			chan.cwrite(buf2, len);
			// recieve response
			chan.cread(buf3, file_req->length);
			// write buf3 into a new file:
			secondFile.write(buf3, file_req->length);

			currSize -= max;
		}
		// close file
		secondFile.close();

		// get rid of the buffers to not have memory leaks
		delete[] buf2;
		delete[] buf3;
	}

	if (new_chan) {
		MESSAGE_TYPE m = QUIT_MSG;
		newChan->cwrite(&m, sizeof(MESSAGE_TYPE));
		delete newChan;
	}
	
	// closing the channel    
    MESSAGE_TYPE m = QUIT_MSG;
    chan.cwrite(&m, sizeof(MESSAGE_TYPE));

	waitpid(server_id, &status, 0); // Wait then end. 
	
}
