#include <fstream>
#include <iterator>
#include <vector>
#include <iostream>
#include <string>
#include <map>
#include <algorithm>
#include <iterator>



using namespace std;
//structs
struct MMU
{
    unsigned long logicalAddress;
    unsigned long offset;
    unsigned long pageNumber;
    MMU(unsigned long n){
        logicalAddress=n;
        offset = n & 255;
        pageNumber = (n & 65280) >> 8;
    }
};


//calling functions


unsigned long pageFault(unsigned long int pageNumber,vector<pair<unsigned long int,unsigned long int>>&TLB,vector<pair<unsigned long int,unsigned long int>>&pageTable,map <unsigned long int,vector<long int>>&physicalMemory,vector<int>buffer);
void update(unsigned long int pageNumber,unsigned int long frameNumber, vector<pair<unsigned long int,unsigned long int>>&pairVector,unsigned long long n);
long int readPhysicalMemory(unsigned long int frameNumber,unsigned long int offset,map <unsigned long int,vector< long int>>&physicalMemory);
bool checker(unsigned long int pageNumber, map <unsigned long int,vector<long int>>&physicalMemory,unsigned long int offset,unsigned long int logicalAddress, vector<pair<unsigned long int,unsigned long int>>&table,long int i);
void lastReport(int pageFaultCounter,int addressReadCounter,int tlbHitCounter );
void report(unsigned long int second,unsigned long int logicalAddress,unsigned long int offset,long int i,map <unsigned long int,vector<long int>>&physicalMemory);
string decToBinary(unsigned long int n,int c);



int main()
{
    ifstream addressFile("addresses.txt");
    ifstream input( "BACKING_STORE.bin", ios::binary );

    vector<int> buffer(std::istreambuf_iterator<char>(input), {});// it copies all the information of "BACKING_STORE.bin" file, into vector buffer, just for speeding up the program.
    map <unsigned long int,vector<long int>>physicalMemory;//as it was mentioned in "OS-Spring2021-p3.pdf", it is a dictionary(map). it is empty initially and in each pagefault it will fill after each framenumber it isn't in physical memory
    vector<pair<unsigned long int,unsigned long int>>TLB;//as it was mentioned in "OS-Spring2021-p3.pdf", it is a list(vector). it is empty initially
    vector<pair<unsigned long int,unsigned long int>>pageTable;//as it was mentioned in "OS-Spring2021-p3.pdf", it is a list(vector). it is empty initially
    long int pageFaultCounter = 0;//it shows how many page fault we had
    long int tlbHitCounter = 0;//it shows how many tlb hits these addresses had and then for finding the tlb hit rate
    long int addressReadCounter = 0;// this is used for the first numbers in "output.txt" file

    remove( "output.txt" );//it is an append option for file so we delete the previous information if it exists.


    string line;
    while (getline (addressFile, line)) { //for each logical address in "addresses.txt" file do:
        bool tlbHit=0 ; //it shows if tlbhit happened or not
        bool pageTableTrue = 0;

        MMU logicalAddress (unsigned(stoi(line,nullptr,10))) ;

        addressReadCounter += 1;

        tlbHit = checker(logicalAddress.pageNumber, physicalMemory, logicalAddress.offset, logicalAddress.logicalAddress, TLB, addressReadCounter);//it will report the physical address if there was in tlb

        if( tlbHit )
        {
            tlbHitCounter += 1; //if we had a tlb hit so the tlb counter increase in 1

        }

        else
        {
            pageTableTrue = checker(logicalAddress.pageNumber, physicalMemory, logicalAddress.offset, logicalAddress.logicalAddress, pageTable, addressReadCounter);//otherwise , we should checl page table

            if (!pageTableTrue  )//if it was in neither tlb nor page table so a page fault occurs
            {
                unsigned long frameNumber=pageFault(logicalAddress.pageNumber, TLB, pageTable, physicalMemory,buffer);
                pageFaultCounter += 1;
                report(frameNumber,logicalAddress.logicalAddress,logicalAddress.offset,addressReadCounter,physicalMemory);

            }
        }

    }
    lastReport( pageFaultCounter, addressReadCounter, tlbHitCounter );
    addressFile.close();


}


//methods


unsigned long int pageFault(unsigned long int pageNumber,vector<pair<unsigned long int,unsigned long int>>&TLB,vector<pair<unsigned long int,unsigned long int>>&pageTable,map <unsigned long int,vector<long int>>&physicalMemory,vector<int>buffer){
    string frameNumber="h";//initilize with a not related string
    unsigned long int frameNumbern=257;//initilize with an outer bound

    if (pageNumber<256)
    {
        //we should find corresponding data in "BACKING_STORE.bin".
        //we will search for missing data in physical memory and fix it
        //we will make framenumber corresponging to empty position
        for(unsigned long int i=0;i<256;i++)
            if(physicalMemory.find(i)==physicalMemory.end())
            {
                frameNumber=to_string(i);
                frameNumbern=i;
                break;
            }
        if(frameNumber!="h")
        {
            physicalMemory[frameNumbern]={};
            for (unsigned long int i=0;i<256;i++)
            {
                physicalMemory[frameNumbern].push_back(buffer[pageNumber*256+i]);

            }
            update(pageNumber, frameNumbern, TLB,16);
            update(pageNumber, frameNumbern, pageTable,256);
        }
        else {
            cout<<"Page \" "<< pageNumber << "\" is out of bound!"<<endl;
        }
    }
    return frameNumbern;
}

void update(unsigned long int pageNumber,unsigned int long frameNumber, vector<pair<unsigned long int,unsigned long int>>&pairVector,unsigned long long n)
//if there the capacity (the capacity is arguman n) is not full we can push to the table
//if the capacity is full we delete the first data. everytime we had a paget table hit or tlb hit the information will erase and readd at the end of the table(->see updateLRU function)
{
   if( pairVector.size() < n)
    {
        pairVector.push_back(make_pair(pageNumber,frameNumber));
    }
    else
    {
        pairVector.erase(pairVector.begin());
        pairVector.push_back(make_pair(pageNumber,frameNumber));

    }
}
void updateLRU(unsigned long int latestEntryIndex,vector<pair<long unsigned int,long unsigned int>>&pairVector)
{
    pair<long int,long int> latestEntry = pairVector[latestEntryIndex];
    pairVector.erase(pairVector.begin()+latestEntryIndex);
    pairVector.push_back(latestEntry);

}
long int readPhysicalMemory(unsigned long int frameNumber,unsigned long int offset,map <unsigned long int,vector< long int>>&physicalMemory)
{
    //we will see what is the information in the frame number we got as argumant
    //it would accelarate the speed in comparison of getting data from "BACKING_STORE.bin"
    long int data=0;
    if (frameNumber < 256 and offset < 256)
    {
        data = physicalMemory[frameNumber][offset];

    }

    else
    {
        cout<<"Frame number or offset is out of bound"<<endl;

    }

    return data;
}

bool checker(unsigned long int pageNumber, map <unsigned long int,vector<long int>>&physicalMemory,unsigned long int offset,unsigned long int logicalAddress, vector<pair<unsigned long int,unsigned long int>>&table,long int i)
{
    //it will recognize if the pagenumber exists in table or not
    //if it was exists in the table it will show the information corresponding to that in "output.txt" data
   for (unsigned long int j=0; j<table.size();j++)
    {
        if (pageNumber == table[j].first)
        {
            report(table[j].second,logicalAddress,offset,i,physicalMemory);
            updateLRU(j, table);
            return 1;
        }
    }
    return 0;
}
string decToBinary(unsigned long int n,int c)
{
    //it will transform n from decimal to binaty and it will have(c- number of n's bits) 0 for upper weigh bits
    string binary;
    for (int i = c-1; i >= 0; i--) {
        unsigned int k = n >> i;
        if (k & 1)
            binary.append("1");
        else
            binary.append("0");
    }
    return binary;
}
void report(unsigned long int second,unsigned long int logicalAddress,unsigned long int offset,long int i,map <unsigned long int,vector<long int>>&physicalMemory){
    long int data;
    ofstream outfile;
    outfile.open("output.txt", ios_base::app);
    unsigned long int frameNumber = second;
    data=readPhysicalMemory(frameNumber, offset, physicalMemory);
    //physical address is frame number and offset. it is obvios why we use decToBinary function with that zeros. the reson is it's offset that we need to be 8 bits for changing in decimal
    string physicalAddress = decToBinary(frameNumber,8) +decToBinary(offset,8);
    outfile<< i<<" Virtual address: " <<logicalAddress<< " Physical address: " <<
              stoi(physicalAddress,nullptr,2)<< " Value: " << data <<"\n";
    outfile.close();

}
void lastReport(int pageFaultCounter,int addressReadCounter,int tlbHitCounter )
{
    //the final report about pagefaults tlb hits tlb hits rate and...
    ofstream outputFile;
    outputFile.open("output.txt", ofstream::app);
    double pageFaultRate = double(pageFaultCounter) / double(addressReadCounter);
    double tlbHitRate = double(tlbHitCounter) / double(addressReadCounter);
    outputFile<< "Number of translated address: "<<addressReadCounter<< '\n' << "Number of page fault: " <<
                 pageFaultCounter << '\n' << "Page fault rate: " << pageFaultRate << '\n' << "Number of TLB hits: " <<
                 tlbHitCounter << '\n' << "TLB hit rate: "  << tlbHitRate<< '\n';

    outputFile.close();
}
