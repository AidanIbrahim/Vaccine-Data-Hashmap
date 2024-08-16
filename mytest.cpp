// CMSC 341 - Spring 2024 - Project 4
#include "vacdb.h"
#include <math.h>
#include <random>
#include <vector>
#include <algorithm>
#include <ctime>     //used to get the current time

const char* PASS = "        \e[4;32mTEST PASSED\x1b[0m"; //These are for pass fail message formatting
const char* FAIL = "        \e[1;91mTEST FAILED: \x1b[0m";

unsigned int hashCode(const string str);

unsigned int hashCode(const string str) {
   unsigned int val = 0 ;
   const unsigned int thirtyThree = 33 ;  // magic number from textbook
   for (unsigned int i = 0 ; i < str.length(); i++)
      val = val * thirtyThree + str[i] ;
   return val;
}

unsigned int hashCodeDebug(const string str);

unsigned int hashCodeDebug(const string str) { //This is a garbage function by design, it will always cause the key to go to index 0
   unsigned int val = 0 ;                      //This is useful for viewing the probing in a more systematic way
   return val;
}


// We can use the Random class to generate the test data randomly!
enum RANDOM {UNIFORMINT, UNIFORMREAL, NORMAL, SHUFFLE};
class Random {
public:
    Random(int min, int max, RANDOM type=UNIFORMINT, int mean=50, int stdev=20) : m_min(min), m_max(max), m_type(type)
    {
        if (type == NORMAL){
            //the case of NORMAL to generate integer numbers with normal distribution
            m_generator = std::mt19937(m_device());
            //the data set will have the mean of 50 (default) and standard deviation of 20 (default)
            //the mean and standard deviation can change by passing new values to constructor 
            m_normdist = std::normal_distribution<>(mean,stdev);
        }
        else if (type == UNIFORMINT) {
            //the case of UNIFORMINT to generate integer numbers
            // Using a fixed seed value generates always the same sequence
            // of pseudorandom numbers, e.g. reproducing scientific experiments
            // here it helps us with testing since the same sequence repeats
            m_generator = std::mt19937(10);// 10 is the fixed seed value
            m_unidist = std::uniform_int_distribution<>(min,max);
        }
        else if (type == UNIFORMREAL) { //the case of UNIFORMREAL to generate real numbers
            m_generator = std::mt19937(10);// 10 is the fixed seed value
            m_uniReal = std::uniform_real_distribution<double>((double)min,(double)max);
        }
        else { //the case of SHUFFLE to generate every number only once
            m_generator = std::mt19937(m_device());
        }
    }
    void setSeed(int seedNum){
        // we have set a default value for seed in constructor
        // we can change the seed by calling this function after constructor call
        // this gives us more randomness
        m_generator = std::mt19937(seedNum);
    }

    void getShuffle(vector<int> & array){
        // the user program creates the vector param and passes here
        // here we populate the vector using m_min and m_max
        for (int i = m_min; i<=m_max; i++){
            array.push_back(i);
        }
        shuffle(array.begin(),array.end(),m_generator);
    }

    void getShuffle(int array[]){
        // the param array must be of the size (m_max-m_min+1)
        // the user program creates the array and pass it here
        vector<int> temp;
        for (int i = m_min; i<=m_max; i++){
            temp.push_back(i);
        }
        std::shuffle(temp.begin(), temp.end(), m_generator);
        vector<int>::iterator it;
        int i = 0;
        for (it=temp.begin(); it != temp.end(); it++){
            array[i] = *it;
            i++;
        }
    }

    int getRandNum(){
        // this function returns integer numbers
        // the object must have been initialized to generate integers
        int result = 0;
        if(m_type == NORMAL){
            //returns a random number in a set with normal distribution
            //we limit random numbers by the min and max values
            result = m_min - 1;
            while(result < m_min || result > m_max)
                result = m_normdist(m_generator);
        }
        else if (m_type == UNIFORMINT){
            //this will generate a random number between min and max values
            result = m_unidist(m_generator);
        }
        return result;
    }

    double getRealRandNum(){
        // this function returns real numbers
        // the object must have been initialized to generate real numbers
        double result = m_uniReal(m_generator);
        // a trick to return numbers only with two deciaml points
        // for example if result is 15.0378, function returns 15.03
        // to round up we can use ceil function instead of floor
        result = std::floor(result*100.0)/100.0;
        return result;
    }

    string getRandString(int size){
        // the parameter size specifies the length of string we ask for
        // to use ASCII char the number range in constructor must be set to 97 - 122
        // and the Random type must be UNIFORMINT (it is default in constructor)
        string output = "";
        for (int i=0;i<size;i++){
            output = output + (char)getRandNum();
        }
        return output;
    }
    
    private:
    int m_min;
    int m_max;
    RANDOM m_type;
    std::random_device m_device;
    std::mt19937 m_generator;
    std::normal_distribution<> m_normdist;//normal distribution
    std::uniform_int_distribution<> m_unidist;//integer uniform distribution
    std::uniform_real_distribution<double> m_uniReal;//real uniform distribution

};
class Tester{ 

    public:
    
        bool constructionTest(){
            VacDB h1 = VacDB(1, hashCode, LINEAR);
            if (h1.m_currentCap != MINPRIME){
                cout << FAIL << "A size below minprime does not make the table size minprime" << endl;
                return false;
            } else if (h1.m_currProbing != LINEAR || h1.m_oldTable != nullptr || h1.m_currentSize != 0 || h1.m_currNumDeleted != 0){
                cout << FAIL << "Member variables are not correctly initialized" << endl;
                return false;
            }

            VacDB h2 = VacDB(MAXPRIME*5, hashCode, QUADRATIC);

            if (h2.m_currentCap != MAXPRIME){
                cout << FAIL << "A size above maxprime does not make the table size maxprime" << endl;
                return false;
            }

            VacDB h4 = VacDB(MINPRIME+1, hashCode, QUADRATIC);

            if (!(h4.isPrime(h2.m_currentCap))){
                cout << FAIL << "A non-prime input does not result in a prime sized table" << endl;
                return false;
            }

            return true;
        }

        bool insertRemoveTest(int size, vector<Patient>& patientList){
            VacDB h1 = VacDB(500, hashCode, QUADRATIC); //Allocates a MAXPRIME hashtable because I don't want to worry about rehashes just yet
            //First I will test a single instance of insert and remove, then the test will do TEST_SIZE entries
            if (!h1.insert(patientList[0])){
                cout << FAIL << "A single insert has failed" << endl;
                return false;
            }

            if (!(patientList[0] == h1.getPatient(patientList[0].getKey(), patientList[0].getSerial()))){
                cout << FAIL << "A single get has failed" << endl;
                return false;
            }

            if (h1.insert(patientList[0])){
                cout << FAIL << "A duplicate insert was allowed" << endl;
                return false;
            }
            
            //Would test a single remove but that would make the deleted ratio 1 and that would cause a rehash
            //Rehash isn't tested yet so we should avoid doing it


            //TEST_SIZE patient test

            for (int i=1;i<size; i++){ //Testing insert en masse, start i at one since we already inserted the first patient on the list
                Patient testPatient = patientList[i];
                if (!h1.insert(testPatient)){
                    cout << FAIL << "A mass insert has failed at least once" << endl; //Fail if it returns false
                    return false;
                }
                
            }

            for (int i=0;i<size; i++){
            // generating random data
                Patient testPatient = patientList[i];
                if (!(h1.getPatient(patientList[i].getKey(), patientList[i].getSerial()) == patientList[i])){ //Mass get test
                    cout << FAIL << "Mass get has failed, or inserted data was not found" << endl;
                    return false;
                }
                
            }
            
            for (int i=0;i<size*0.79; i++){ //Multiply by 0.79 to prevent a rehash from deleted ratio
                if (!h1.remove(patientList[i])){
                    cout << FAIL << "Mass remove has failed at least once" << endl;
                    return false;
                }
            }
            
            Patient notInArray("Not a generated name", 1000, true);
            if (h1.remove(notInArray)){
                cout << FAIL << "Remove returned true after a non existent element was passed in" << endl;
                return false;
            }
            
            for (int i=0;i<size*0.79; i++){ //Multiply by 0.79 to prevent a rehash from deleted ratio
                if (!(h1.getPatient(patientList[i].getKey(), patientList[i].getSerial()) == Patient())){
                    cout << FAIL << "Trying to get a deleted element does not return an empty patient" << endl;
                    return false;
                }
            }

            return true;
        }

        bool updatesSerialTest(int size, vector<Patient>& patientList){
            VacDB h1 = VacDB(MAXPRIME, hashCode, DOUBLEHASH);
            for (int i=0;i<size; i++){ //Making the table
                Patient testPatient = patientList[i]; //Constructing the VacDB
                h1.insert(testPatient);        
            }

            for (int i=0;i<size; i++){  //Updating all serial numbers
                Patient testPatient = patientList[i]; 
                if(!h1.updateSerialNumber(testPatient, 7777)){ //Will update every patients serial number then check to see if we find any that aren't changed
                    cout << FAIL << "An updateSerialNumber() has failed" << endl;
                    return false;
                }      
            }

            for (int i=0;i<h1.m_currentCap; i++){ //Will iterate throuugh the whole table
                if (h1.m_currentTable[i] != nullptr){

                    Patient* testPatient = h1.m_currentTable[i];
                        if(testPatient->getSerial() != 7777){ //Will update every patients serial number then check to see if we find any that aren't changed
                            cout << FAIL << "The information within a patient is incorrect" << endl;
                            return false;
                        }      
                }
            }
            return true;
        }
        
        string getRandName(Random& generator){
            int first = generator.getRandNum();
            int initial = generator.getRandNum(); 
            int last = generator.getRandNum();
            string firstName = firstNames[(first % 50)];
            char mInitial = initials[(initial % 26)];
            string lastName = lastNames[(last % 50)];

            string fullName = firstName + " " + mInitial + " " + lastName;
            return fullName;
        }

        bool lambdaTest(int size, vector<Patient>& patientList){
            VacDB h1(MAXPRIME, hashCode, DOUBLEHASH); //Again, using max size to avoid rehash

            float expected = (float)size/MAXPRIME;
            if (h1.lambda() != 0){ //Empty case
                cout << FAIL << "The returned lambda value does not work with an empty table" << endl;
                return false;
            }

            for (int i = 0; i < size; i++){
                h1.insert(patientList[i]); //Insert elements
            }
            if (h1.lambda() != expected){
                cout << FAIL << "The returned lambda value was incorrect" << endl;
                return false;
            } else {
                return true;
            }
        }

        bool testDelRatio(int size, vector<Patient>& patientList){
            VacDB h1(MAXPRIME, hashCode, DOUBLEHASH); //Again, using max size to avoid rehash

            if (h1.deletedRatio() != 0){ //Empty case
                cout << FAIL << "The returned deleted ratio was incorrect (empty case)" << endl;
                return false;
            }

            for (int i = 0; i < size; i++){
                h1.insert(patientList[i]); //Insert elements
            }

            for (int i = 0; 4*i < size; i++){ //Remove a quarter of the paients, this will keep a rehash from triggering
                h1.remove(patientList[i]); //Insert elements
            }

            float expected = 0.25;
            if (h1.deletedRatio() != expected){
                cout << FAIL << "The returned deleted ratio was incorrect" << endl;
                return false;
            } else {
                return true;
            }
        }

        bool rehashTest(int size, vector<Patient> patientList){
            VacDB h1(MINPRIME, hashCode, DOUBLEHASH);
            int index = 0;
            for (int i = 0; i < h1.m_currentCap/2; i++){ //Do half of the table to prevent a rehash, we will check the rehash
                h1.insert(patientList[i]); //Insert elements
                index++; //Saving the last use index for later
            }
            int rehashSize = h1.findNextPrime(4*((h1.m_currentCap/2))); // This will be used later to check the size of the next table
            //Now I will insert one at a time and search for every inserted patient to verify
            h1.insert(patientList[index]);
            if (h1.m_oldTable == nullptr){
                cout << FAIL << "The table was not rehashed when the lambda reached 0.5" << endl;
                return false;
            }
             for (int i = 0; i < index; i++){ //Do half of the table to prevent a rehash, we will check the rehash
               if (h1.getPatient(patientList[i].getKey(), patientList[i].getSerial()) == Patient()){
                cout << FAIL << "A patient was not found during the rehash" << endl;
                return false;
               }

            }
            index++;
            h1.insert(patientList[index]); //Progress the search again
            for (int i = 0; i < index; i++){ //Do half of the table to prevent a rehash, we will check the rehash
               if (h1.getPatient(patientList[i].getKey(), patientList[i].getSerial()) == Patient()){
                cout << FAIL << "A patient was not found during the rehash" << endl;
                return false;
               }

            }
            index++;
            h1.insert(patientList[index]); //Progress the search again
            for (int i = 0; i < index; i++){ //Do half of the table to prevent a rehash, we will check the rehash
               if (h1.getPatient(patientList[i].getKey(), patientList[i].getSerial()) == Patient()){
                cout << FAIL << "A patient was not found during the rehash" << endl;
                return false;
               }

            }
            index++;
            h1.insert(patientList[index]); //Progress the search again
            for (int i = 0; i < index; i++){ //Do half of the table to prevent a rehash, we will check the rehash
               if (h1.getPatient(patientList[i].getKey(), patientList[i].getSerial()) == Patient()){
                cout << FAIL << "A patient was not found during the rehash" << endl;
                return false;
               }

            }

            if (h1.m_oldTable != nullptr){
                cout << FAIL << "Old table was not deallocated after rehashing" << endl;
                return false;
            }

            if (h1.m_currentSize != index+1){ //Add one to index variable since it started at 0
                cout << FAIL << "The current size private variable is incorrect" << endl;
                return false;
            }
            
            if (h1.m_currentCap != rehashSize){ //Add one to index variable since it started at 0
                cout << h1.m_currentCap << ":::" << rehashSize << endl;
                cout << FAIL << "The current cap of the new table is incorrect" << endl;
                return false;
            }


            return true;
        }
        
        bool rehashRemove(int size, vector<Patient> patientList){
            VacDB h1(MINPRIME, hashCode, DOUBLEHASH);
            for (int i = 0; i < size; i++){
                h1.insert(patientList[i]);
            }

            for (int i = 0; i < size; i++){ //Do half of the table to prevent a rehash, we will check the rehash
               if (h1.getPatient(patientList[i].getKey(), patientList[i].getSerial()) == Patient()){
                cout << FAIL << "A patient was not found during the rehash" << endl;
                return false;
               }

            }
            
            int saveIndex = 0; //Saves the index
            for (int i = 0; i < size-1; i++){ //remove all but one data point
                h1.remove(patientList[i]);
                saveIndex = i+1;
            }
            
            if (Patient() == h1.getPatient(patientList[saveIndex].getKey(), patientList[saveIndex].getSerial())){
                cout << FAIL << "A patient was not preserved" << endl;
                return false;
            }

            if (h1.m_currentSize != 1){
                cout << FAIL << "m_currSize was not allocated correctly after removal rehash" << endl;
                return false;
            }

            if (h1.m_currentCap != MINPRIME){ //Make sure table waa shrunk when points were removed
                cout << FAIL << "The cap of the table is incorrect" << endl;
                return false;
            }

            return true;
        }
        
        bool changeProbeTest(int size, vector<Patient> patientList){
            VacDB h1(MINPRIME, hashCodeDebug, LINEAR);
            int index = 0;
            for (int i = 0; i < h1.m_currentCap/2; i++){ //Keep from rehashing
                h1.insert(patientList[i]); //Insert elements
                index++; //Saving the last use index for later
            }
            int longestCluster = 0; //The linear probe with the dummy hash should cause one big cluster, so if after rehash the cluster is not as large,
                                    //the probing change has taken effect
            while (h1.m_currentTable[longestCluster] != nullptr){
                longestCluster++;
            }
            
            h1.changeProbPolicy(QUADRATIC);
            h1.insert(patientList[index]);
            h1.insert(patientList[index+1]);
            h1.insert(patientList[index+2]);
            h1.insert(patientList[index+3]);

            for (int i = 0; i < longestCluster; i++){ //If the one big cluster is the same or bigger, the quadratic probing wasn't used
                if (h1.m_currentTable[i] == nullptr){
                    return true;
                }
            }
            cout << FAIL << "The probing type was not changed properly" << endl;
            return false;
        }

        


        //Random name database
        vector<std::string> firstNames = {
            "Alice", "Bob", "Charlie", "David", "Emma", "Frank", "Grace", "Henry", "Isabella", "Jack",
            "Kate", "Liam", "Mia", "Noah", "Olivia", "Patrick", "Quinn", "Rachel", "Samuel", "Taylor",
            "Ursula", "Vincent", "Wendy", "Xander", "Yvonne", "Zachary", "Abigail", "Benjamin", "Catherine",
            "Daniel", "Ella", "Felix", "Gabriella", "Hannah", "Isaac", "Jessica", "Kylie", "Lucas", "Madison",
            "Nathan", "Oliver", "Penelope", "Quentin", "Rebecca", "Sophia", "Thomas", "Victoria", "William", "Iris", "Firstnamius"
        };

        std::vector<char> initials = {
            'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
            'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z'
        };

        vector<std::string> lastNames = {
        "Smith", "Johnson", "Williams", "Jones", "Brown", "Davis", "Miller", "Wilson", "Moore", "Taylor",
        "Anderson", "Thomas", "Jackson", "White", "Harris", "Martin", "Thompson", "Garcia", "Martinez", "Robinson",
        "Clark", "Rodriguez", "Lewis", "Lee", "Walker", "Hall", "Allen", "Young", "Hernandez", "King",
        "Wright", "Lopez", "Hill", "Scott", "Green", "Adams", "Baker", "Gonzalez", "Nelson", "Carter",
        "Mitchell", "Perez", "Roberts", "Turner", "Phillips", "Campbell", "Parker", "Evans", "Edwards", "Lastnamius"
    };
};


//These names will be used to generate names of students

int main(){
    const int TEST_SIZE = 100;
    Patient empty = Patient();
    Tester test; //Awful variable name but saves line space

    vector<Patient> patientList;
    Random RndID(MINID,MAXID);
    Random RndName(0,50);// selects one from the namesDB array
    Random RndQuantity(0,50);
    VacDB vacdb(MINPRIME, hashCode, LINEAR);

    
    cout << "\e[0;35mHash Table Project Test Suite\e[0;97m" << endl;
    cout << "\e[1;34mSection 1: Construction and Insertion\e[0;97m" << endl;
    cout << "   1.1: Construction Test" << endl;

    //This test checks that the construction is working properly, and checks the member variables of VacDB
    if (test.constructionTest()){
        cout << PASS << endl;
    }
    cout << "   1.2: Insert-Get-Remove Test" << endl;
    
    for (int i=0;i<TEST_SIZE; i++){
        // generating random data
            Patient testPatient = Patient(test.getRandName(RndName), RndID.getRandNum(), true);
            patientList.push_back(testPatient);
        }

    //This test tests the remove function, both with a single patient, and a mass amount (TEST_SIZE) of them after.
    if (test.insertRemoveTest(TEST_SIZE, patientList)){
        cout << PASS << endl;
    }
    cout << "\e[1;34mSection 2: lambda(), deletedRatio() and updateSerial\e[0;97m" << endl;
    cout << "   2.1: Lambda Test" << endl;

    //This test checks the lambda returns the correct fuction, both an empty case and a normal case
    if (test.lambdaTest(TEST_SIZE, patientList)){
        cout << PASS << endl;
    }
    
    cout << "   2.2: Deleted Ratio Test" << endl;
    
    //This tests the deleted ratio function for both an empty case and a normal case
    if (test.testDelRatio(TEST_SIZE, patientList)){
        cout << PASS << endl;
    }
    
    cout << "   2.3: Update serial" << endl;

    //This function tests the update serial funtion with a large amount of patients to make sure collisions are a factor
    if (test.updatesSerialTest(TEST_SIZE, patientList)){
        cout << PASS << endl;
    }

    cout << "\e[1;34mSection 3: Rehashing\e[0;97m" << endl;
    cout << "   3.1: Rehash lambda case" << endl;

    //This function tests the basic portions of the incremental rehash, and a load factor triggered rehash
    if (test.rehashTest(TEST_SIZE, patientList)){
        cout << PASS << endl;
    }

    cout << "   3.2: Rehash remove case" << endl;
    
    //This function tests a deleted ratio triggered rehash
    if (test.rehashRemove(TEST_SIZE, patientList)){
        cout << PASS << endl;
    }

    cout << "   3.3: New probing type request" << endl;

    //This function tests the change probing policy function during the rehash
    if (test.changeProbeTest(TEST_SIZE, patientList)){
        cout << PASS << endl;
    }
    cout << "\e[0;35mTest Suite has run to completion\e[0;97m" << endl;
    return 0;
}

