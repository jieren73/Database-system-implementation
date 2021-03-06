#include <iostream>
#include <fstream>
#include <sstream>			// used for var to str conversion
#include <string.h>			// used for memcpy and memmove
#include "RelOp.h"
#include "EfficientMap.h"
#include "EfficientMap.cc"
#include "Keyify.h"
#include "Config.h"
using namespace std;

ostream& operator<<(ostream& _os, RelationalOp& _op) {
	return _op.print(_os);
}
int counterEQ = 0;
void QueryExecutionTree::ExecuteQuery() {
	Record rec;
	// Delete Me
	//cout << "Execution Tree Counter: " << counterEQ++ << endl;
	while (root->GetNext(rec)) {	//call getNext of root until there are no more tuples
		//cout << "Execution Tree Counter: " << counterEQ++ << endl;
	}
}

Scan::Scan(Schema& _schema, DBFile& _file, string table) {
	schema = _schema;
	s = _schema;
	file = _file;
	this->table = table;
	counter = 0;
}

bool Scan::GetNext(Record& _record) {
	//cout << "Scan Counter: " << counter++ << endl;
	//if (counter > 100)
	//	exit(0);
	if(file.GetNext(_record)) {
		return true;
	}
	return false;
}

Scan::Scan()
{
	schema = Schema();
	file =  DBFile();
	counter = 0;
}

Scan::~Scan() {

}

Schema& Scan::getSchema()
{
	return schema;
}

DBFile& Scan::getFile()
{
	return file;
}

string Scan::getTable() {
	return table;
}

void Scan::Swap(Scan& withMe)
{
	SWAP(schema, withMe.getSchema());
	SWAP(file, withMe.getFile());
}

void Scan::CopyFrom(Scan& withMe)
{
	this->schema = withMe.schema;
	this->file = withMe.file;
}

ostream& Scan::print(ostream& _os) {
	return _os << "SCAN\nSchema:"<<schema<<endl;
}

Select::Select(Schema& _schema, CNF& _predicate, Record& _constants,
	RelationalOp* _producer, string table) {
	schema = _schema;
	s = _schema;
	predicate = _predicate;
	constants = _constants;
	producer = _producer;
	this->table = table;

	counter = 0;
}

Select::Select()
{
	counter = 0;
}

Select::~Select() {

}

Schema& Select::getSchema()
{
	return schema;
}

CNF& Select::getCNF()
{
	return predicate;
}

Record& Select::getRecord()
{
	return constants;
}

RelationalOp* Select::getRelational()
{
	return producer;
}

string Select::getTable() {
	return table;
}

void Select::Swap(Select& withMe)
{
	SWAP(schema, withMe.schema);
	SWAP(predicate, withMe.predicate);
	SWAP(constants, withMe.constants);
	SWAP(producer, withMe.producer);
}

ostream& Select::print(ostream& _os) {
	return _os << "SELECT\nSchema: "<<schema<<"\nPredicate: "<<predicate<<"\nProducer: " << *producer << endl;
}

bool Select::GetNext(Record& _record) {
	//Record record;
	//cout << "Select Counter: " << counter++ << endl;
	while (producer->GetNext(_record)) {
		if (predicate.Run(_record, constants)) {	// constants = literals?
			//_record = record;
			return true;
		}
	}
	return false;
}

Project::Project(Schema& _schemaIn, Schema& _schemaOut, int _numAttsInput,
	int _numAttsOutput, int* _keepMe, RelationalOp* _producer) {
	s = _schemaOut;
	schemaIn = _schemaIn;
	schemaOut = _schemaOut;
	numAttsInput = _numAttsInput;
	numAttsOutput = _numAttsOutput;
	keepMe = _keepMe;
	producer = _producer;
	counter = 0;
}

Project::~Project() {

}

ostream& Project::print(ostream& _os) {
	_os << "PROJECT: \nSchema In: " << schemaIn << "\nSchema Out: " << schemaOut << "\n# Attributes Input: " << numAttsInput << "\n# Attributes Output: " << numAttsOutput << "\nKeep: ";
	for (int i = 0; i < numAttsOutput; i++) {
		_os << keepMe[i] << " ";
	}
	return _os << "\nProducer: " << *producer << endl;
}

bool Project::GetNext(Record& _record) {
	// Assume Project is working correctly
	// that is every private member variable is holding what it describes in header file
	if (producer->GetNext(_record)) {
		_record.Project(keepMe, numAttsOutput, numAttsInput);
		return true;
	}
	return false;
}

Join::Join(Schema& _schemaLeft, Schema& _schemaRight, Schema& _schemaOut,
	CNF& _predicate, RelationalOp* _left, RelationalOp* _right) {
	schemaLeft = _schemaLeft;
	schemaRight = _schemaRight;
	schemaOut = _schemaOut;
	s = _schemaOut;
	predicate = _predicate;
	left = _left;
	right = _right;
	appendIndex = 0;
	buildCheck = true;
}

Join::~Join() {

}

bool Join::GetNext(Record& _record) {
//Nested-Loop Join
	if (buildCheck) {

		buildCheck = false;
		Record temp;

		//Build
		while (right->GetNext(temp)) {

			Record* temp2 = new Record();

			*temp2 = temp;

			myDS.push_back(temp2);

		}

		// Probe

		while (left->GetNext(temp)) {


			for (int i = 0; i < myDS.size(); i++) {

				if (predicate.Run(temp, *myDS[i])) {

					Record merge;
					merge.AppendRecords(temp, *myDS[i], schemaLeft.GetNumAtts(), schemaRight.GetNumAtts());

					Record* merge2 = new Record();
					*merge2 = merge;

					appendRecords.push_back(merge2);
				}
			}
		}

	}


	if (appendIndex < appendRecords.size()) {

		_record = *appendRecords[appendIndex];
		appendIndex++;
		return true;

	}

	else {

		return false;
	}

}


Schema & Join::getSchema(){
	return schemaOut;
}

ostream& Join::print(ostream& _os) {
	return _os << "JOIN\nLeft Schema:\n" << schemaLeft << "\nRight Schema:\n" << schemaRight << "\nSchema Out:\n" << schemaOut << "\nPredicate:\n" << predicate << "\nLeft Operator:\n{\n" << *left << "\n}\nRight Operator:\n{\n" << *right << "\n}" << endl;
}


DuplicateRemoval::DuplicateRemoval(Schema& _schema, RelationalOp* _producer, OrderMaker &_duplComp) {
	schema = _schema;
	producer = _producer;
	check = true;
	duplComp.Swap(_duplComp);
}

DuplicateRemoval::~DuplicateRemoval() {

}

ostream& DuplicateRemoval::print(ostream& _os) {
	return _os << "DISTINCT \nSchema: " << schema << "\nProducer: " << *producer << endl;
}

bool DuplicateRemoval::GetNext(Record& _record)//compiles but is not finished
{
	//cout << "hi" << endl;
	if (check)
	{
	//	cout << "Making" << endl;
		Record recTemp;
		producer->GetNext(recTemp);
	//	cout << schema << endl;
		recTemp.SetOrderMaker(&duplComp);
	//	cout << "ordered" << duplComp << duplComp.numAtts<<endl;
	//	recTemp.Project(duplComp.whichAtts, duplComp.numAtts, schema.GetNumAtts());
	//	Record *makerPtr = new Record();
	//	*makerPtr = recTemp;
	//	cout << "Pushing first" << endl;
		KeyInt AMARSUCKSCOCK = KeyInt(1);
		duplTemp.Insert(recTemp, AMARSUCKSCOCK);
	//	cout << "Pushed" << endl;
		check = false;
	//	it = 0;//global iterator
	//	cout << "entering" << endl;
		Record recTemp2;
		while (producer->GetNext(recTemp2))
		{
			AMARSUCKSCOCK = KeyInt(1);
			recTemp2.SetOrderMaker(&duplComp);
	//		cout << duplComp<< endl;
			//recTemp2.Project(duplComp.whichAtts, duplComp.numAtts, schema.GetNumAtts());
	//		cout << "filling" << i << endl;
	//		Record *RecPtr = new Record();
	//		cout << "Amar" << endl;
	//		*RecPtr = recTemp2;
	//		cout << "cumdumpster" << i;
		/*	duplTemp[i]->print(cout, schema);
			cout << endl;
			cout<< " COMPARING " <<endl;
			recTemp2.print(cout,schema);
			cout << endl;*/
	//		recTemp2.print(cout, schema);
	//		cout << endl;
	//		cout << schema << endl;
			if (!duplTemp.IsThere(recTemp2))
			{
	//			cout << "faggot" << i<<endl;
				duplTemp.Insert(recTemp2,AMARSUCKSCOCK);
			}
	//		cout << "no homo" << i<<endl;
		}
		duplTemp.MoveToStart();
	}
//	cout << "Run " << it << endl;
	if (duplTemp.AtEnd())
	{
		return false;
	}
	else
	{
		_record=duplTemp.CurrentKey();
		duplTemp.Advance();
		return true;
	}

}

// Slow Variable to String Conversion Method
// Alternative Options (may require downloading):
// http://stackoverflow.com/questions/191757/how-to-concatenate-a-stdstring-and-an-int
template <typename T>
string convert(T x)
{
	ostringstream convert;   			// stream used for the conversion
	convert << x;		      			// insert the textual representation of 'Number' in the characters in the stream
	return convert.str(); 				// set 'Result' to the contents of the stream
}

Sum::Sum(Schema& _schemaIn, Schema& _schemaOut, Function& _compute,
	RelationalOp* _producer) {
	schemaIn = _schemaIn;
	schemaOut = _schemaOut;
	compute = _compute;
	producer = _producer;
	first = true;
}

Sum::~Sum() {

}

// TEST WITH:
//		- Queries/Phase4Queries/1.sql, 2, 5, 9, 11-16
bool Sum::GetNext(Record & _record)
{
	Record temp;
	int valI; double valD;
	double runningSum = 0;

	if (first) {
		while (producer->GetNext(temp)) {

			compute.Apply(temp, valI, valD);

			if (compute.GetSumType() == 1) {		// int
				runningSum += valI;
			}
			else if (compute.GetSumType() == 0) {	// double
				runningSum += valD;
			}
		}
		first = false;
	}
	else {
		return false;
	}

	// create recrod with running sum
	// Create bits
	double doubletemp = runningSum;
	// size = (size of bits) + (location of double) + (size of double)
	int bitsize = sizeof(int) + sizeof(int) + sizeof(double);
	char* bits = new char[bitsize];								// temp storage
																// columnLoc = (size of bits) + (location of double)
	int columnLoc = sizeof(int) + sizeof(int);					// column location
	*((double *) &(bits[columnLoc])) = doubletemp;				// insert sum
	((int *)bits)[0] = bitsize;									// size of bits
	((int *)bits)[1] = columnLoc;								// location of the sum

	_record.Consume(bits);										// Define _record (SUM Record)
	delete bits;												// Delete bits

	/*
	FILE* fp;
	string s;
	string separator = convert('|');

	s = convert(runningSum) + separator;
	char* str = new char[s.length() + 1];			// string to char* converter
	strcpy(str, s.c_str());
	fp = fmemopen(str, s.length() * sizeof(char), "r");

	//Extract the "FILE"
	vector<int> x;
	x.push_back(0);
	Schema sumz = schemaOut;
	sumz.Project(x);
	_record.ExtractNextRecord(sumz, *fp);
	*/
	return true;
}

ostream& Sum::print(ostream& _os) {
	return _os << "SUM\nSchemaIn: " << schemaIn << "\nSchemaOut: " << schemaOut << " Function" << "Producer: " << *producer << endl;//told by TA to just use "Function"
}


GroupBy::GroupBy(Schema& _schemaIn, Schema& _schemaOut, OrderMaker& _groupingAtts,
	Function& _compute,	RelationalOp* _producer) {
	schemaIn = _schemaIn;
	schemaOut = _schemaOut;
	groupingAtts.Swap(_groupingAtts);
	compute = _compute;
	producer = _producer;
	first = true;
	counter = 0;
}


// NOW SUPPORTS ALL POSSIBLE GROUPING ATTRIBUTES AND SUMS
// KEY = Record(GROUPING ATTRIBUTE); DATA = SUM
// TEST WITH:
//		- Queries/Phase4Queries/11.sql for <int, float>
//		- Queries/Phase4Queries/12.sql for <string, float>
//		- Queries/Phase4Queries/13.sql for <float, float>
//		- Queries/Phase4Queries/14.sql for <int, int>
//		- Queries/Phase4Queries/15.sql for <string, int>
//		- Queries/Phase4Queries/16.sql for <float, int>
bool GroupBy::GetNext(Record& _record) {

	if (first)																	// check if this is called the first time
	{
		Record temp, projtemp;													// Records to insert
		int intResult = 0; double doubleResult = 0;										// for Function's Apply (computing sum)
		int returnsInt;															// to determine whether the sum type is int
		double sum;																// SUM to insert
		KeyDouble td;															// temp KeyDouble
		while (producer->GetNext(temp))
		{
			returnsInt = compute.Apply(temp, intResult, doubleResult);			// compute sum
			sum = intResult + doubleResult;										// get sum
			projtemp = temp;													// extract only grouping attributes from Record
			projtemp.SetOrderMaker(&groupingAtts);
			td = KeyDouble(sum);
			if (map.IsThere(projtemp))											// the map has the record
				map.Find(projtemp) = KeyDouble(map.Find(projtemp) + sum);
			else																// the map doesn't have the record
				map.Insert(projtemp, td);										// insert into map
			intResult = 0; doubleResult = 0;
		}
		first = false;															// set first time to false
		map.MoveToStart();
	}

	if (map.AtEnd())
		return false;
	Record r1, r2;
	char* c;
	int size;

	// Create a FILE here
	/*FILE* fp;
	string s;
	string separator = convert('|');
	double doubletemp = map.CurrentData();
	s = convert(doubletemp) + separator;
	char* str = new char[s.length() + 1];					// string to char* converter
	strcpy(str, s.c_str());
	fp = fmemopen(str, s.length() * sizeof(char), "r");

	//Extract the "FILE"

	vector<int> x;
	x.push_back(0);
	Schema sumz = schemaOut;
	sumz.Project(x);
	r1.ExtractNextRecord(sumz, *fp);
	*/

	// Create bits
	double doubletemp = map.CurrentData();
	// size = (size of bits) + (location of double) + (size of double)
	int bitsize = sizeof(int) + sizeof(int) + sizeof(double);
	char* bits = new char[bitsize];								// temp storage
	// columnLoc = (size of bits) + (location of double)
	int columnLoc = sizeof(int) + sizeof(int);					// column location
	*((double *) &(bits[columnLoc])) = doubletemp;				// insert sum
	((int *)bits)[0] = bitsize;									// size of bits
	((int *)bits)[1] = columnLoc;								// location of the sum

	r1.Consume(bits);											// Define r1 (SUM Record)
	delete bits;												// Delete bits

	// Create r2 (GroupingAtts)
	map.CurrentKey().Project(groupingAtts.whichAtts, groupingAtts.numAtts, schemaIn.GetNumAtts());
	c = map.CurrentKey().GetBits();
	size = map.CurrentKey().GetSize();
	r2.CopyBits(c, size);

	_record.AppendRecords(r1, r2, 1, groupingAtts.numAtts);		// Merge 2 records

	//fclose(fp);
	//delete str;
	//delete c;
	map.Advance();
	return true;
}

GroupBy::~GroupBy() {

}

ostream& GroupBy::print(ostream& _os) {
	return _os << "GROUP BY\nSchemaIn: " << schemaIn << "\nSchemaOut: " << schemaOut << "\nGroupingAtts: " << groupingAtts << "\nFunction:\nFunction\n" << "Producer:\n{\n" << *producer << "\n}" << endl;//told by TA to just use "Function"
}


WriteOut::WriteOut(Schema& _schema, string& _outFile, RelationalOp* _producer) {
	schema = _schema;
	s = _schema;
	outFile = _outFile;
	producer = _producer;
	counter = 0;
}

WriteOut::~WriteOut() {

}

bool WriteOut::GetNext(Record& _record) {
	//ofstream myOutputFile;								// Unn, no difference
	//myOutputFile.open(outFile.c_str());					// Unn, no difference

	// Delete Me
	//cout << "Write Out Counter: " << counter++ << endl;
	if (producer->GetNext(_record)) {
		_record.print(cout, schema);
		cout << endl;
		//myOutputFile.close();								// Unn, no difference

		return true;
	}

	//myOutputFile.close();									// Unn, no difference
	return false;
}

ostream& WriteOut::print(ostream& _os) {
	return _os << "OUTPUT\nSchema: " << schema << "\nOut File: " << outFile << "\nProducer:\n{\n" << *producer << "\n}\n";
}


ostream& operator<<(ostream& _os, QueryExecutionTree& _op) {
	return _os << "QUERY EXECUTION TREE" << "(\n" << *_op.root << "\n)";
	//return _os << endl;
}
