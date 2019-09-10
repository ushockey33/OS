/* 
 * Scheduling
 * Sean Dubiel
 */ 

#include <fstream>
#include <string>
#include <iostream>
#include <limits>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <vector>
#include <bits/stdc++.h> 
#include <memory>
#include <cmath>
#include <deque>
#include <algorithm>
#include <map> 

using namespace std;

//OPTIMIZATION could optimize by removing variables that will not be used for certain schedules by creating individual process structs for each schedule.
//OPTIMIZATION could also change type from into to possibly short or other type for certain varibles that will not need the range.
struct process {
	int pid, bst, arr, pri, priio, dline, io, ioold, bstold, start, end;
	process(int _pid, int _bst, int _arr, int _pri, int _dline, int _io, int _start){
		pid = _pid;
		bst = _bst;
		arr = _arr;
		pri = _pri;
		priio = pri;
		dline = _dline;
		io = _io;
		ioold = io;
		bstold = bst;
		start = _start;
		end = 0;
	}
};
bool processRTSSorter(process p1, process p2){
	//TODO maybe have to check pri before pid
	if(p1.arr == p2.arr){
		if(p1.dline == p2.dline){
			return (p1.pid < p2.pid);
		}
		return (p1.dline < p2.dline);	
	}
	return (p1.arr < p2.arr);	
}
bool processRTSQueueSorter(process p1, process p2){
	//TODO maybe have to check pri before pid
	if(p1.dline == p2.dline){
		return (p1.pid < p2.pid);
	}
	return (p1.dline < p2.dline);	
}
bool processMFQSSorter(process p1, process p2){
	if(p1.arr == p2.arr){
		if(p1.pri == p2.pri){
			return (p1.pid < p2.pid);
		}
		return (p1.pri < p2.pri);	
	}
	return (p1.arr < p2.arr);	
}
bool processMFQSQueueSorter(process p1, process p2){
	if(p1.pri == p2.pri){
		return (p1.pid < p2.pid);
	}
	return (p1.pri < p2.pri);	
}
bool allQueuesEmpty(vector<deque<process> >& queuesystem){
	for(int i = 0; i<queuesystem.size();i++){
		if( !queuesystem[i].empty() ){
			return false;
		}
	}
	return true;
}
/*
*returns true if the queuesystem is empty
*/
bool allQueuesEmptyWHS(vector<deque<process> >& lowband, vector<deque<process> >& highband){
	for (int i=99; i >=50; i--) {
		if( !(highband[i-50].empty()) ) {
			return false;
		}
	}
	for (int j=49; j >=0; j--) {
		if( !(lowband[j].empty()) ) {
			return false;
		}
	}
	return true;
}
/*
*Returns highest priority queue's index with > 0 processes in it or -1 for empty for the MFQS queuesystem
*/
int findTopQueue(vector<deque<process> >& queuesystem) {
	for (int i=0; i<queuesystem.size(); i++) {
		if( !(queuesystem[i].empty()) ) {
			return i;
		}
	}
	return -1;
}
/*
*Returns highest priority queue's index with > 0 processes in it or -1 for empty for the WHS queuesystem
*/
int findTopQueueWHS(vector<deque<process> >& lowband, vector<deque<process> >& highband) {
	for (int i=99; i >=50; i--) {
		if( !(highband[i-50].empty()) ) {
			return i;
		}
	}
	for (int j=49; j >=0; j--) {
		if( !(lowband[j].empty()) ) {
			return j;
		}
	}
	return -1;
}
/*
*Takes in a queue matrix, queue to be aged, and the age interval.
*It will not age the queue if it is the highest queue in the matrix
*/
void increaseAge(vector<deque<process> >& queuesystem, int qnum, int ageintval, int topQ){
	
	if(qnum > 0){
		for(int i = 0; i < queuesystem[qnum].size();i++){
			//make sure not adding age to torun process
			if( !(topQ == qnum && i == 0) ) {
				queuesystem[qnum][i].start = queuesystem[qnum][i].start + 1;
				//promote
				//TODO This may be ageintval +1
				if(queuesystem[qnum][i].start == ageintval+1){
					//move to back of higher queue and erase from old queue
					#ifdef DEBUG
					cout <<"AGED UP"<<	"\n";
					#endif
					queuesystem[qnum][i].start = 0;
					queuesystem[qnum-1].push_back(queuesystem[qnum][i]);
					queuesystem[qnum].erase(queuesystem[qnum].begin() + i);
					
				}
			}
		}
	}	
}
/*
*Takes in a queue matrix, queue num to be aged, and the age interval.
*It will not age the queue if it is the highest queue in the matrix
*/
void increaseAgeWHS(vector<deque<process> >& lowband, int ageintval, int topQ){
	for(int i = 0; i < 10;i++){
		for(int j=0;j<lowband[i].size();j++){
			//make sure we are not aging a running bottom 10 process
			if( !(i == topQ && j == 0)){
					#ifdef DEBUG
					cout <<"	AGED + PID "<<lowband[i][j].pid<<" age "<<lowband[i][j].start<<" agveintval "<<ageintval<< "\n";
					#endif
				lowband[i][j].start = lowband[i][j].start + 1;
				//TODO This may be ageintval +1
				if(lowband[i][j].start == ageintval+1){
					#ifdef DEBUG
					cout <<"AGED UP PID "<<lowband[i][j].pid<<"\n";
					#endif
					lowband[i][j].start = 0;
					lowband[i+10].push_back(lowband[i][j]);
					lowband[i].erase(lowband[i].begin() + j);
				}
			}
		}
	}
}
/*
*Takes in a low and high band queue matrix, queue num to be aged, and the age interval.
*It will not age the queue if it is the highest queue in either band.
*/
void demoteWHS(vector<deque<process> >& lowband, vector<deque<process> >& highband, int topQ, int clock){
	
	int priSet = 0;
	if(topQ - 1 < 10){ //if we are demoting to lowest queue then increase its age 
		lowband[topQ][0].start = 1;
	}
	if(topQ <50){ //if running in lowest queue
		priSet = topQ - (lowband[topQ][0].bstold - lowband[topQ][0].bst);
		if(priSet < lowband[topQ][0].pri){ 
			priSet = lowband[topQ][0].pri;
		}
		#ifdef DEBUG
		cout << "	DEMOTE: Clock " << clock <<  " pid " << lowband[topQ][0].pid << " burst " << lowband[topQ][0].bst << " priset "<< priSet<<" pri "<<lowband[topQ][0].pri<<"\n";
		#endif
		lowband[topQ][0].start = 0;
		lowband[priSet].push_back(lowband[topQ][0]);
		lowband[topQ].pop_front();
	}else{ // any other queue
		priSet = topQ - (highband[topQ-50][0].bstold - highband[topQ-50][0].bst);
		if(priSet < highband[topQ-50][0].pri){ 
			priSet = highband[topQ-50][0].pri;
		}
		#ifdef DEBUG
		cout << "	DEMOTE: Clock " << clock <<  " pid " << highband[topQ-50][0].pid << " burst " << highband[topQ-50][0].bst << " priSet " << priSet<<" pri "<<highband[topQ-50][0].pri<<"\n";
		#endif
		highband[topQ-50][0].start = 0;
		highband[priSet-50].push_back(highband[topQ-50][0]);
		highband[topQ-50].pop_front();
	}
	
}
/*
*Takes in the low and high band queue systems along with the iolist. Processes in the iolist will have their io fields
*decremented until 0 is reached. They will then be added back to the either the low or high band based on the specifications outlined in our associated paper.
*/
void increaseIO(vector<deque<process> >& lowband, vector<deque<process> >& highband, deque<process>& iolist){
	if(!iolist.empty()){
		for(int i = 0; i<iolist.size();i++){
			iolist[i].io = iolist[i].io -1;
			//TODO should this be -1 like increase age?
			if(iolist[i].io == 0){
				int prinum = iolist[i].priio + iolist[i].ioold;
				if(iolist[i].priio < 50){
					
					if(prinum > 49){
						prinum = 49;
					}
					iolist[i].start = 0;
					#ifdef DEBUG
					cout << "	IO ADD: Clock " <<  " pid " << iolist[i].pid << " burst " << iolist[i].bst << " priSet " << prinum<< " "<<"\n";
					#endif
					lowband[prinum].push_back(iolist[i]);
					iolist.erase(iolist.begin() + i);
					
				}else{
					if(prinum > 99){
						prinum = 99;
					}
					iolist[i].start = 0;
					#ifdef DEBUG
					cout << "	IO ADD: "<<  " pid " << iolist[i].pid << " burst " << iolist[i].bst << " priSet " << prinum<< " "<<"\n";
					#endif
					highband[prinum-50].push_back(iolist[i]);
					iolist.erase(iolist.begin() + i);
				}
			}
		}
	}	
}
/*
*Real Time Scheduling Simulation based on specifications listed in the associated paper.
*Takes in file of input in format "Pid	Bst	Arr	Pri Ddline	I/O" serpated by tabs and then newlines.
*/
int scheduleRTS(string file_name, bool hardenv){
	int pid;
    int bst;
	int arr;
	int pri;
	int dline;
	int io;
	string str;
	vector<process> plist;
	vector<process> queue;
	int processdone = 0;
	float totalwaittime = 0;
	float totalturnaroundtime = 0;
	int clock = 0;
	int currindex = 0;
	bool found = false;
	#ifdef CHART
	fstream output;
	output.open ("ganntChartRTS.out",fstream::out);
	output<<"Gannt Chart\n Clock | PID\n";
	#endif

	ifstream file(file_name.c_str());
	if(!file.good()){
		cout << "File not found" << '\n';
		return 1;
	}
	getline(file, str); //grab first line of strings
	//Store each line in vector of process structs
	while (file >> pid >> bst >> arr >> pri >> dline >> io ){
		//remove any processes where deadline is negative, deadline is greater than arrival, arr+bst > deadline, or bst is less than 1
		if( (dline >= 0) && (dline > arr) &&  (bst + arr) <= dline && (bst > 0) ){
			plist.push_back(process(pid,bst,arr,pri,dline,io,-1));
		}else{
			if(hardenv){
				cout << "EXIT: Will not finish in hard env before first clock cycle - pid " << pid << " burst " << bst << " arr "<< arr << " dline " << dline <<"\n";
				return 1;
			}else{
				#ifdef DEBUG
				cout << "Will not finish in soft env before first clock cycle - pid " << pid << " burst " << bst << " arr "<< arr << " dline " << dline <<"\n";
				#endif
			}
		}
		
	}
	
	cout<< "\033[4;31mAdd process: \"Pid Bst Arr Pri Ddline I/O\" type done when finished \033[0m\n";	
	while (cin >> pid >> bst >> arr >> pri >> dline >> io ){	
		//remove any processes where deadline is negative, deadline is greater than arrival, arr+bst > deadline, or bst is less than 1
		if( (bst > 0) && pri>=0 &&pri<100&& io>=0 && dline>=0){
			plist.push_back(process(pid,bst,arr,pri,dline,io, 0));
			cout <<"Added: PID "<< pid <<" burst "<< bst <<" arrival "<< arr <<" priority "<< pri <<" deadline "<< dline <<" io "<< io <<"\n";
		}	
	}
	
	sort(plist.begin(),plist.end(), processRTSSorter);
	cout<< "\033[4;31mStart RTS with "<<plist.size()<<" processes\n\033[0m";
	cout<< "Waiting...\n";
	//start rts scheduling 
	//TODO gather stats turnaround time, wait time, time in cpu
	//The time spent in the CPU of the failed process should be included in the calculation of the average turnaround time and waiting time for the scheduler.
	//each clock simulates 1 clock tick


	while( currindex < plist.size() || !queue.empty() ){
		
		#ifdef CHART
		output<<"\n" <<clock<< " | ";
		#endif
		//add all processes for this clock cycle
		while (currindex < plist.size()){
			if(plist[currindex].arr == clock){
				queue.push_back(plist[currindex]);
			}else if(plist[currindex].arr > clock){
				break;
			}
			currindex++;
		}
		
		if(!queue.empty()){
			//remove all that will not finish
			for(int i = 0; i<queue.size();i++){
				
				if( ((queue[i].bst) + clock) > (queue[i].dline) ){
					#ifdef DEBUG
					cout << "Will not finish in soft env - clock is "<<clock<<" pid " << queue[i].pid << " burst " << queue[i].bst << " arr "<< queue[i].arr << " dline " << queue[i].dline <<"\n";
					#endif
					if(hardenv){
						cout << "EXIT: Will not finish in hard env - pid " << queue[i].pid << " burst " << queue[i].bst << " arr "<< queue[i].arr << " dline " << queue[i].dline << " clock " << clock << " " <<"\n";
						return 1;
					}
					queue[i].end = clock;
					//TODO send process for data collection here
					queue.erase(queue.begin() + i);
				}
			}
			//sort by  deadline then pid
			sort(queue.begin(),queue.end(), processRTSQueueSorter);
			//"run" 1st process in queue
			queue[0].bst = queue[0].bst - 1;
			#ifdef CHART
			output<<queue[0].pid;
			#endif
			#ifdef DEBUG
			cout << "	RUN Clock is "<<clock<<" pid " << queue[0].pid << " burst " << queue[0].bst << " arr "<< queue[0].arr << " dline " << queue[0].dline <<"\n";
			#endif
			if(queue[0].start == -1){
				queue[0].start = clock;
			}
			//process is complete
			if(queue[0].bst == 0){
				totalwaittime = (((clock+1) - queue[0].arr) - queue[0].bstold) + totalwaittime;
				totalturnaroundtime  = ((clock+1) - queue[0].arr) + totalturnaroundtime;
				queue[0].end = clock;
				processdone++;
				#ifdef DEBUG
				cout << "DONE Clock " << clock <<  " pid " << queue[0].pid << " arr "<< queue[0].arr << " waittime "<<(((clock+1) - queue[0].arr) - queue[0].bstold) <<" turnaroundtime "<<((clock+1) - queue[0].arr) <<"\n\n";
				#endif
				//TODO send process for data collection here
				//cout << "DONE Clock " << clock <<  " pid " << queue[0]->pid << " burst " << queue[0]->bst << " arr "<< queue[0]->arr << " dline " << queue[0]->dline << " "<<"\n";
				queue.erase(queue.begin());
			}
		}
		clock++;
	}
	#ifdef CHART
	output.close();
	#endif
	cout <<"\033[1;36m\nClock ends at "<< clock << " processes done " <<processdone<<"\n";
	cout <<"Total wait time "<< totalwaittime << " total turnaround time " <<totalturnaroundtime<<"\n";
	float awt = totalwaittime/processdone ;
	float att = totalturnaroundtime/processdone;
	cout <<"Average wait time "<< awt << " Average turnaroud time " <<att<<"\n\n\033[0m";

	return 0;
}
/*
*Real Time Scheduling Simulation based on specifications listed in the associated paper.
*Takes in file of input in format "Pid	Bst	Arr	Pri Ddline	I/O" serpated by tabs and then newlines.
*/
int scheduleMFQS(string file_name, int upperbound, int timequantum, int ageintval){
	int pid;
    int bst;
	int arr;
	int pri;
	int dline;
	int io;
	string str;
	vector<process> plist;		
	vector<deque<process> > queuesystem(upperbound, deque<process>() );
	int processdone = 0;
	float totalwaittime = 0;
	float totalturnaroundtime = 0;
	#ifdef CHART
	fstream output;
	output.open ("ganntChartMFQS.out",fstream::out);
	output<<"Gannt Chart\n Clock | PID\n";
	#endif
	
	ifstream file(file_name.c_str());
	if(!file.good()){
		cout << "File not found" << '\n';
		return 1;
	}	
	getline(file, str); //grab first line of strings
	
	//Store each line in vector of process structs
	while (file >> pid >> bst >> arr >> pri >> dline >> io ){		
		//remove any processes where  or bst is less than 1
		if( (bst > 0) && dline>=0){//remove all fields that have negative
			plist.push_back(process(pid,bst,arr,pri,dline,io, 0));
		}	
	}
	cout<< "\033[4;31mAdd process: \"Pid Bst Arr Pri Ddline I/O\" type done when finished \033[0m\n";	
	while (cin >> pid >> bst >> arr >> pri >> dline >> io ){	
		//remove any processes where deadline is negative, deadline is greater than arrival, arr+bst > deadline, or bst is less than 1
		if( (bst > 0) && pri>=0 &&pri<100&& io>=0 && dline>=0){
			plist.push_back(process(pid,bst,arr,pri,dline,io, 0));
			cout <<"Added: PID "<< pid <<" burst "<< bst <<" arrival "<< arr <<" priority "<< pri <<" deadline "<< dline <<" io "<< io <<"\n";
		}	
	}
	
	
	sort(plist.begin(), plist.end(), processMFQSSorter);
	
	int clock = 0;
	int currindex = 0;
	int TQ = 0;
	int topQ = -1; //index row for torun process
	int y = 0; //index column for torun process
	int count =0;
	cout<< "\033[4;31mStart MFQS with "<<plist.size()<<" processes\n\033[0m";
	cout<< "Waiting...\n";
	while(currindex < plist.size() || !allQueuesEmpty(queuesystem)/*clock < 50000*/){
		#ifdef CHART
		output<<"\n" <<clock<< " | ";
		#endif
		//Add all elements at current clock cycle
		while (currindex < plist.size()){
			if(plist[currindex].arr == clock){
				queuesystem[0].push_back(plist[currindex]);
				count++;
			}else if(plist[currindex].arr > clock){
				break;
			}
			currindex++;
		}
		
		if(count != 0){

			//cout <<"Clock "<< clock << " TQ " <<TQ<< " topQ "<<topQ <<"\n\n";
			if(TQ != 0){
				
				increaseAge(queuesystem, upperbound - 1, ageintval, topQ);
				//"run" process
				if(topQ == upperbound-1){ //if running in lowest queue
					queuesystem[topQ][0].bst = queuesystem[topQ][0].bst -1;
					//TQ--;
				}else{ // any other queue
					queuesystem[topQ][0].bst = queuesystem[topQ][0].bst -1;
					TQ--;
				}
				#ifdef CHART
				output<<queuesystem[topQ][0].pid;
				#endif
				#ifdef DEBUG
				cout << "	RUN Clock " << clock <<  " pid " << queuesystem[topQ][0].pid << " burst " << queuesystem[topQ][0].bst << " arr "<< queuesystem[topQ][0].arr<<"\n";
				#endif
			}else if(TQ == 0){
				increaseAge(queuesystem, upperbound - 1, ageintval, topQ);
				
				//demote unless torun is empty
				//iter_swap(v.begin() + position, v.begin() + nextPosition);
				if(topQ !=-1 && upperbound != 1 && topQ != upperbound -1){ //torun is empty and we are not running in a 1 queue system and we are not in the last queue
					//demote
					if(topQ + 1 == upperbound -1){ //if we are demoting to lowest queue then increase its age 
						queuesystem[topQ][0].start = 1;
					}
					queuesystem[topQ+1].push_back(queuesystem[topQ][0]);
					queuesystem[topQ].pop_front();
				}
				//set new top process
				topQ = findTopQueue(queuesystem);
				//"run" process
				if(topQ == upperbound-1){ //if running in lowest queue
					TQ = queuesystem[topQ][0].bst -1;
					queuesystem[topQ][0].bst = queuesystem[topQ][0].bst -1;
					
				}else if(topQ !=-1){ // any other queue
					TQ = ( (pow(2,topQ) * timequantum)-1 );
					//TQ== 16;
					queuesystem[topQ][0].bst = queuesystem[topQ][0].bst -1;
				}
				#ifdef CHART
				output<<queuesystem[topQ][0].pid;
				#endif
				#ifdef DEBUG
				cout << "NEW RUN Clock " << clock <<  " pid " << queuesystem[topQ][0].pid << " burst " << queuesystem[topQ][0].bst << " arr "<< queuesystem[topQ][0].arr << " "<<"\n";
				#endif
			}
			//if we finish the burst before our allocated TQ 
			if(topQ != -1){
				if(queuesystem[topQ][0].bst == 0){
					processdone++;					
					totalwaittime = (((clock+1) - queuesystem[topQ][0].arr) - queuesystem[topQ][0].bstold) + totalwaittime;
					totalturnaroundtime  = ((clock+1) - queuesystem[topQ][0].arr) + totalturnaroundtime;
					#ifdef DEBUG
					cout << "DONE Clock " << clock <<  " pid " << queuesystem[topQ][0].pid << " arr "<< queuesystem[topQ][0].arr << " waittime "<<((clock+1) - queuesystem[topQ][0].arr) <<" turnaroundtime "<< (((clock+1) - queuesystem[topQ][0].arr) - queuesystem[topQ][0].bstold) <<"\n\n";
					#endif
					//collect stats here.
					//queuesystem[topQ].erase(queuesystem[topQ].begin());
					queuesystem[topQ].pop_front();
					count--;
					//reset
					TQ = 0;
					topQ = -1;
				}	
			}			
		}
		clock++;
	}
	#ifdef CHART
	output.close();
	#endif
	cout <<"\033[1;36m\nClock ends at "<< clock << " processes done " <<processdone<<"\n";
	cout <<"Total wait time "<< totalwaittime << " total turnaround time " <<totalturnaroundtime<<"\n";
	float awt = totalwaittime/processdone ;
	float att = totalturnaroundtime/processdone;
	cout <<"Average wait time "<< awt << " Average turnaroud time " <<att<<"\n\n\033[0m";
}
/*
*Windows Hybrid Scheduling Simulation based on specifications listed in the associated paper.
*Takes in file of input in format "Pid	Bst	Arr	Pri Ddline	I/O" serpated by tabs and then newlines.
*/
int scheduleWHS(string file_name, int timequantum, int ageintval){
	int pid;
    int bst;
	int arr;
	int pri;
	int dline;
	int io;
	string str;
	vector<process> plist;		
	vector<deque<process> > highband(50, deque<process>() );
	vector<deque<process> > lowband(50, deque<process>() );
	deque<process> iolist; //would be better as priority queue
	int clock = 0;
	int currindex = 0;
	int TQ = 0;
	int topQ = -1; //index row for torun process
	int count =0;
	int processdone = 0;
	int qset = 0;
	float totalwaittime = 0;
	float totalturnaroundtime = 0;
	#ifdef CHART
	fstream output;
	output.open ("ganntChartWHS.out",fstream::out);
	output<<"Gannt Chart\n Clock | PID\n";
	#endif
	
	
	ifstream file(file_name.c_str());
	if(!file.good()){
		cout << "File not found" << "\n";
		return 1;
	}	
	getline(file, str); //grab first line of strings
	
	//Store each line in vector of process structs
	while (file >> pid >> bst >> arr >> pri >> dline >> io ){		
		//remove any processes where deadline is negative, deadline is greater than arrival, arr+bst > deadline, or bst is less than 1
		if( (bst > 0) && pri>=0 &&pri<100&& io>=0 && dline>=0){
			plist.push_back(process(pid,bst,arr,pri,dline,io, 0));
		}	
	}
	
	cout<< "\033[4;31mAdd process: \"Pid Bst Arr Pri Ddline I/O\" type done when finished \033[0m\n";	
		while (cin >> pid >> bst >> arr >> pri >> dline >> io ){	
		//remove any processes where deadline is negative, deadline is greater than arrival, arr+bst > deadline, or bst is less than 1
		if( (bst > 0) && pri>=0 &&pri<100&& io>=0 && dline>=0){
			plist.push_back(process(pid,bst,arr,pri,dline,io, 0));
			cout <<"Added: PID "<< pid <<" burst "<< bst <<" arrival "<< arr <<" priority "<< pri <<" deadline "<< dline <<" io "<< io <<"\n";
		}	
	}
	//add user input here
	//same sort as mfqs
	sort(plist.begin(), plist.end(), processMFQSSorter);

	cout<< "\033[4;31mStart WHS with "<<plist.size() <<" processes\033[0m\n";
	cout<< "Waiting...\n";

	while(currindex < plist.size() || !allQueuesEmptyWHS(lowband, highband) || !iolist.empty()){
	#ifdef CHART
	output<<"\n" <<clock<< " | ";
	#endif
		//Add all elements at current clock cycle
		while (currindex < plist.size()){
			if(plist[currindex].arr == clock){
				qset = plist[currindex].pri;
				if(plist[currindex].pri < 50){
					lowband[qset].push_back(plist[currindex]);
				}else{
					highband[qset-50].push_back(plist[currindex]);
				}
				
				count++;
			}else if(plist[currindex].arr > clock){
				break;
			}
			currindex++;
		}
		//End of adding
		if(count != 0){	
			//add from io and tick away for io, treat same as ageincrease
			increaseIO(lowband, highband, iolist);
			//check for io here TODO does age go before io promotion
			if(TQ == 1){
				int iocheck = 0;
				if(topQ <50){ //if running in lowest queue
					iocheck = lowband[topQ][0].io;
					if(iocheck != 0){
					//send to io
						lowband[topQ][0].priio = topQ;
						
						iolist.push_back(lowband[topQ][0]);
						lowband[topQ].pop_front();
						TQ=0;
						topQ = -1;
					}
				}else{ // any other queue
					iocheck = highband[topQ-50][0].io;
					if(iocheck != 0){
					//send to io
						highband[topQ-50][0].priio = topQ;
						iolist.push_back(highband[topQ-50][0]);
						highband[topQ-50].pop_front();
						TQ=0;
						topQ = -1;						
					}
				}		
			}
			if(TQ != 0){
				increaseAgeWHS(lowband, ageintval, topQ);
				//"run" process
				if(topQ <50){ //if running in lowest queue
					lowband[topQ][0].bst = lowband[topQ][0].bst -1;
					#ifdef DEBUG
					cout << "	RUN Clock " << clock <<  " pid " << lowband[topQ][0].pid << " burst " << lowband[topQ][0].bst << " arr "<< lowband[topQ][0].arr << " dline " << lowband[topQ][0].dline << " "<<"\n";
					#endif
					#ifdef CHART
					output<<lowband[topQ][0].pid;
					#endif
				}else{ // any other queue
					highband[topQ-50][0].bst = highband[topQ-50][0].bst -1;
					#ifdef DEBUG
					cout << "	RUN Clock " << clock <<  " pid " << highband[topQ-50][0].pid << " burst " << highband[topQ-50][0].bst << " arr "<< highband[topQ-50][0].arr << " dline " << highband[topQ-50][0].dline << " "<<"\n";
					#endif
					#ifdef CHART
					output<<highband[topQ-50][0].pid;
					#endif
				}
				TQ--;
			}else if(TQ == 0){
				increaseAgeWHS(lowband,  ageintval, topQ);
				//demote unless torun is empty and dont demote if in last queue of high or low band
				if(topQ !=-1  && topQ != 0 && topQ != 50){ //torun is empty and we are not running in a 1 queue system and we are not in the last queue
					//demote
					demoteWHS(lowband, highband, topQ, clock);
				}
				//set new top process
				topQ = findTopQueueWHS(lowband, highband);
				//"run" process
				if(topQ != -1){
					if(topQ <50){ //if running in lowest queue
						lowband[topQ][0].bst = lowband[topQ][0].bst -1;
						TQ = timequantum - 1;
						#ifdef DEBUG
						cout << "NEW RUN Clock " << clock <<  " pid " << lowband[topQ][0].pid << " burst " << lowband[topQ][0].bst << " arr "<< lowband[topQ][0].arr << " dline " << lowband[topQ][0].dline << " "<<"\n";
						#endif
						#ifdef CHART
						output<<lowband[topQ][0].pid;
						#endif
					}else{ // any other queue
						highband[topQ-50][0].bst = highband[topQ-50][0].bst -1;
						TQ = timequantum - 1;
						#ifdef DEBUG
						cout << "NEW RUN Clock " << clock <<  " pid " << highband[topQ-50][0].pid << " burst " << highband[topQ-50][0].bst << " arr "<< highband[topQ-50][0].arr << " dline " << highband[topQ-50][0].dline << " "<<"\n";
						#endif
						#ifdef CHART
						output<<highband[topQ-50][0].pid;
						#endif
					}
				}
			}
			//if we finish the burst before our allocated TQ 
			if(topQ != -1){
				if(topQ <50 ){ //if running in lowest queue
					if(lowband[topQ][0].bst == 0){
						processdone++;
						totalwaittime = (((clock+1) - lowband[topQ][0].arr) - lowband[topQ][0].bstold) + totalwaittime;
						totalturnaroundtime  = ((clock+1) - lowband[topQ][0].arr) + totalturnaroundtime;
						//collect stats here.
						#ifdef DEBUG
						cout << "DONE Clock " << clock <<  " pid " << lowband[topQ][0].pid << " burst " << lowband[topQ][0].bst << " arr "<< lowband[topQ][0].arr << " dline " << lowband[topQ][0].dline << " "<<"\n";
						#endif
						lowband[topQ].pop_front();
						count--;
						//reset
						TQ = 0;
						topQ = -1;
					}
				}else{ // any other queue
					if(highband[topQ-50][0].bst == 0){
						processdone++;
						//collect stats here.
						totalwaittime = (((clock+1) - highband[topQ-50][0].arr) - highband[topQ-50][0].bstold) + totalwaittime;
						totalturnaroundtime  = ((clock+1) - highband[topQ-50][0].arr) + totalturnaroundtime;
						#ifdef DEBUG
						cout << "DONE Clock " << clock <<  " pid " << highband[topQ-50][0].pid << " burst " << highband[topQ-50][0].bst << " arr "<< highband[topQ-50][0].arr << " dline " << highband[topQ-50][0].dline << " "<<"\n";
						#endif
						highband[topQ-50].pop_front();
						count--;
						//reset
						TQ = 0;
						topQ = -1;
					}
				}	
			}			
		}	
		
		clock++;
	}
	
	#ifdef CHART
	output.close();
	#endif
	cout <<"\033[1;36m\nClock ends at "<< clock << " processes done " <<processdone<<"\n";
	cout <<"Total wait time "<< totalwaittime << " total turnaround time " <<totalturnaroundtime<<"\n";
	float awt = totalwaittime/processdone ;
	float att = totalturnaroundtime/processdone;
	cout <<"Average wait time "<< awt << " Average turnaroud time " <<att<<"\n\n\033[0m";
}

int main() { 
	int keeprunning = 1;
	int schedule;
	int upperbound;
	int timequantum;
	int ageintval;
	string file_name;
	bool hardenv;
	
	while(keeprunning){
		
		cout << "\033[4;31mType 1 for rts, 2 for mfqs, 3 for whs, or 4 to exit.\033[0m" << "\n";
		cin >> schedule;
		if( schedule == 1  || schedule == 2 ||schedule == 3){
			cout << "\033[4;31mWhat is the name of your file?\033[0m" << "\n";			
			cin >> file_name;	
		}	
		switch(schedule){
			case 1:
				cout << "\033[4;31mType 0 for a soft environment or 1 for a hard environment.\033[0m" << "\n";			
				cin >> hardenv;
				scheduleRTS(file_name, hardenv);
				break;
				
			case 2:
				cout << "\033[4;31mNumber of queues(1-5): \033[0m" << "\n";
				cin >> upperbound;
				if(upperbound>5 || upperbound < 1){
					cout << "\033[4;31mInvalid number of queues.\033[0m" << "\n";
					break;
				}
				cout << "\033[4;31mFirst queue time quantum: \033[0m" << "\n";
				cin >> timequantum;
				cout << "\033[4;31mAging interval value: \033[0m" << "\n";
				cin >> ageintval;
				scheduleMFQS(file_name, upperbound, timequantum, ageintval);
				break;
				
			case 3:
				cout << "\033[4;31mFirst queue time quantum: \033[0m" << "\n";
				cin >> timequantum;
				cout << "\033[4;31mAging interval value: \033[0m" << "\n";
				cin >> ageintval;
				scheduleWHS(file_name, timequantum, ageintval);
				break;
				
			case 4:
				keeprunning = 0;
				break;
					
			default:
				cout << "\033[4;31mUnknown command\033[0m\n" << '\n';
				break;
				
		}
		cin.clear();
		cin.ignore(numeric_limits<streamsize>::max(), '\n');
		
	}
	
	return 0;
}

