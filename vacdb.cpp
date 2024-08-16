// CMSC 341 - Spring 2024 - Project 4
#include "vacdb.h"


VacDB::VacDB(int size, hash_fn hash, prob_t probing = DEFPOLCY){
    m_hash = hash;
    m_currProbing = probing;
    m_currentSize = 0;
    m_oldTable = nullptr;
    m_transferIndex = 0;
    m_currNumDeleted = 0;
    m_newPolicy = probing;

    if (size >= MINPRIME && size <= MAXPRIME && isPrime(size)){
        m_currentCap = size;
    } else {
        m_currentCap = findNextPrime(size);
    }

    m_currentTable = new Patient*[m_currentCap](); //Allocate memory for hash table
    for (int i = 0; i < m_currentCap; i++){
        m_currentTable[i] = nullptr;
    }
}

void VacDB::checkRehash(){
    if (((lambda() > 0.5 || deletedRatio() > 0.8) || m_transferIndex > 0)){ //If this is true we must rehash, if m_transferindex is non negative rehash is in progress
        rehash();
    }
}

void VacDB::rehash(){
    if (m_transferIndex == 0){ //Initialize all the variables and change the settings
        makeCurrentOld();
        m_currNumDeleted = 0;
        m_currentCap = findNextPrime((m_currentSize - m_oldNumDeleted)*4);
        m_currentTable = new Patient*[m_currentCap]();
        m_currentSize = 0;
        for (int i = 0; i < m_currentCap; i++){ //Make a new empty table
            m_currentTable[i] = nullptr;
        }
    }
    int scanIncrement = m_oldCap / 4;

    Patient newPatient;
    if (m_transferIndex != 3){ //Done rehashing 
        for (int i = scanIncrement*m_transferIndex; i < scanIncrement*(m_transferIndex+1); i++){ //This scans the indexes between the bounds of the quarter
            if (m_oldTable[i] != nullptr){
                if (m_oldTable[i]->getUsed()){
                    newPatient = *m_oldTable[i];
                    transferPatient(newPatient);
                    m_oldTable[i]->setUsed(false);
                }
            }
        }
        m_transferIndex++;
    } else {
        for (int i = scanIncrement*m_transferIndex; i < m_oldCap; i++){ //This scans the indexes between the bounds of the quarter
            if (m_oldTable[i] != nullptr){
                if (m_oldTable[i]->getUsed()){
                    newPatient = *m_oldTable[i];
                    transferPatient(newPatient);
                    m_oldTable[i]->setUsed(false);
                }
            }
        }
        m_transferIndex = 0;
        for (int i = 0; i < m_oldCap; i++){ //This scans the indexes between the bounds of the quarter
            if (m_oldTable[i] != nullptr){
                delete m_oldTable[i];
            }
        }
        delete[] m_oldTable;
        m_oldTable = nullptr;

    }
}

void VacDB::transferPatient(Patient toTransfer){
    unsigned int baseIndex = ((m_hash)(toTransfer.getKey())) % m_currentCap;
    unsigned int index = baseIndex;

    int i = 0; //Start probing at i=0
    while (m_currentTable[index] != nullptr){

        if (m_currentTable[index]->getUsed() == false){ //Overwrite Case
            m_currNumDeleted--;
            assignPatient(toTransfer, m_currentTable[index]);
            return;
        }

        if (*m_currentTable[index] == toTransfer && m_currentTable[index]->getUsed() == true){ //Duplicate found, cancel the insert
            return;
        }

        i++; //Increment, ugly but a while loop was better for this
        index = (baseIndex + probeStep(toTransfer.getKey(), i, m_currProbing)) % m_currentCap; //This probes according to the current method
    }
        
        //Normal case
        m_currentSize++;
        m_currentTable[index] = new Patient();
        assignPatient(toTransfer, m_currentTable[index]);
}

void VacDB::makeCurrentOld(){
    m_oldTable = m_currentTable;
    m_oldCap = m_currentCap;
    m_oldNumDeleted = m_currNumDeleted;
    m_oldProbing = m_currProbing;
    m_currProbing = m_newPolicy;

}

VacDB::~VacDB(){
    for (int i = 0; i < m_currentCap; i++){ //Clear memory
        if (m_currentTable[i] != nullptr){
            delete m_currentTable[i];
        }
    }

    delete[] m_currentTable;

    if (m_oldTable != nullptr){ //If old table exists we must deallocate that too
        for (int i = 0; i < m_oldCap; i++){
            if (m_oldTable[i] != nullptr){
                delete m_oldTable[i];
            }
        }
        delete[] m_oldTable;
    }
}

void VacDB::changeProbPolicy(prob_t policy){
    m_currProbing = policy;
    m_newPolicy = policy;
}

bool VacDB::insert(Patient patient){
    int baseIndex = ((m_hash)(patient.getKey())) % m_currentCap;
    int index = baseIndex;

    int i = 0; //Start probing at i=0
    while (m_currentTable[index] != nullptr){

        if (m_currentTable[index]->getUsed() == false){ //Overwrite Case
            m_currNumDeleted--;
            assignPatient(patient, m_currentTable[index]);
            checkRehash();
            return true;
        }

        if (*m_currentTable[index] == patient && m_currentTable[index]->getUsed() == true){ //Duplicate found, cancel the insert
            checkRehash();
            return false;
        }

        i++; //Increment, ugly but a while loop was better for this
        index = (baseIndex + probeStep(patient.getKey(), i, m_currProbing)) % m_currentCap; //This probes according to the current method
    }
        
        //Normal case
        m_currentSize++;
        m_currentTable[index] = new Patient();
        assignPatient(patient, m_currentTable[index]);
        checkRehash();
        return true;
}

void VacDB::assignPatient(Patient newPatient, Patient* oldPatient){

    oldPatient->setKey(newPatient.getKey());
    oldPatient->setSerial(newPatient.getSerial());
    oldPatient->setUsed(true); //Used in insertion, so this is always true 
}

unsigned int VacDB::probeStep(string patientName, int i, prob_t probType) const {
    unsigned int step = 1; //Will be returned, the number to add to the index
    switch (probType){
        case QUADRATIC:
                step = (i*i); //Quadratic Probe
            break;
        case LINEAR:
                step = i; //Linear Probe
            break;
        case DOUBLEHASH:
            step = i*(11-((m_hash)(patientName)) % 11); //Double Hash
            break;
    }
    return step;
}

bool VacDB::remove(Patient patient){
    unsigned int baseIndex = ((m_hash)(patient.getKey())) % m_currentCap;
    int i = 0; //Incrementor for the loop
    unsigned int index = baseIndex; //Used for calling probeStep
    while (m_currentTable[index] != nullptr){

        if (*m_currentTable[index] == patient && m_currentTable[index]->getUsed()){
            m_currentTable[index]->setUsed(false);
            m_currNumDeleted++;
            checkRehash();
            return true;
        }

        i++;
        index = (baseIndex + probeStep(patient.getKey(), i, m_currProbing)) % m_currentCap; //Increments the current index
    }

    if (m_oldTable != nullptr){
        baseIndex = ((m_hash)(patient.getKey())) % m_oldCap;
        index = baseIndex;
        i = 0; //Reset incrementor
        while (m_oldTable[index] != nullptr){

        if (*m_oldTable[index] == patient && m_oldTable[index]->getUsed()){
            m_oldTable[index]->setUsed(false);
            m_oldNumDeleted++;
            checkRehash();
            return true;
        }

        i++;
        index = (baseIndex + probeStep(patient.getKey(), i, m_oldProbing)) % m_oldCap; //Increments the current index
    }
    }
        checkRehash();
        return false;
}

const Patient VacDB::getPatient(string name, int serial) const{
    int baseIndex = (m_hash)(name) % m_currentCap;
    int index = baseIndex;
    int i = 0;

    Patient searchPatient(name, serial, true); //
    while (m_currentTable[index] != nullptr){

        if (m_currentTable[index]->getUsed()){
            if (*m_currentTable[index] == searchPatient){
                return *m_currentTable[index];
            }
        }
        i++;
        index = (baseIndex + probeStep(searchPatient.getKey(), i, m_currProbing)) % m_currentCap; //Increments the current index
    }

    if (m_oldTable != nullptr){ //Check the old table if it exists
        baseIndex = (m_hash)(name) % m_oldCap;
        index = baseIndex;
        i = 0;
        while (m_oldTable[index] != nullptr){

            if (m_oldTable[index]->getUsed()){
                if (*m_oldTable[index] == searchPatient){
                    return *m_oldTable[index];
                }
            }
            i++;
            index = (baseIndex + probeStep(searchPatient.getKey(), i, m_oldProbing)) % m_oldCap; //Increments the current index
        }
    }

    return Patient();

}

bool VacDB::updateSerialNumber(Patient patient, int serial){
    string name = patient.getKey();
    int baseIndex = (m_hash)(name) % m_currentCap;
    int index = baseIndex;
    int i = 0;

    Patient searchPatient(name, patient.getSerial(), true); 
    while (m_currentTable[index] != nullptr){

        if (m_currentTable[index]->getUsed()){
            if (*m_currentTable[index] == searchPatient){
                m_currentTable[index]->setSerial(serial);
                return true;
            }
        }
        i++;
        index = (baseIndex + probeStep(name, i, m_currProbing)) % m_currentCap; //Increments the current index
    }

    if (m_oldTable != nullptr){ //Check the old table if it exists
        baseIndex = (m_hash)(name) % m_oldCap;
        index = baseIndex;
        i = 0;
        while (m_oldTable[index] != nullptr){

            if (m_oldTable[index]->getUsed()){
                if (*m_oldTable[index] == searchPatient){
                    m_oldTable[index]->setSerial(serial);
                    return true;
                }
            }
            i++;
            index = (baseIndex + probeStep(name, i, m_oldProbing)) % m_oldCap; //Increments the current index
        }
    }

    return false;

}

float VacDB::lambda() const {
    float loadFactor = (float)m_currentSize/m_currentCap;
    return loadFactor;
}

float VacDB::deletedRatio() const {
    if (m_currentSize == 0){
        return 0;
    }
    float entries = m_currentSize;
    float deleted = m_currNumDeleted;
    return deleted/entries;
}

void VacDB::dump() const {
    cout << "Dump for the current table: " << endl;
    if (m_currentTable != nullptr)
        for (int i = 0; i < m_currentCap; i++) {
            cout << "[" << i << "] : " << m_currentTable[i] << endl;
        }
    cout << "Dump for the old table: " << endl;
    if (m_oldTable != nullptr)
        for (int i = 0; i < m_oldCap; i++) {
            cout << "[" << i << "] : " << m_oldTable[i] << endl;
        }
}

bool VacDB::isPrime(int number){
    bool result = true;
    for (int i = 2; i <= number / 2; ++i) {
        if (number % i == 0) {
            result = false;
            break;
        }
    }
    return result;
}

int VacDB::findNextPrime(int current){
    //we always stay within the range [MINPRIME-MAXPRIME]
    //the smallest prime starts at MINPRIME
    if (current < MINPRIME) current = MINPRIME-1;
    for (int i=current; i<MAXPRIME; i++) { 
        for (int j=2; j*j<=i; j++) {
            if (i % j == 0) 
                break;
            else if (j+1 > sqrt(i) && i != current) {
                return i;
            }
        }
    }
    //if a user tries to go over MAXPRIME
    return MAXPRIME;
}

ostream& operator<<(ostream& sout, const Patient* patient ) {
    if ((patient != nullptr) && !(patient->getKey().empty()))
        sout << patient->getKey() << " (" << patient->getSerial() << ", "<< patient->getUsed() <<  ")";
    else
        sout << "";
  return sout;
}

bool operator==(const Patient& lhs, const Patient& rhs){
    // since the uniqueness of an object is defined by name and serial number
    // the equality operator considers only those two criteria
    return ((lhs.getKey() == rhs.getKey()) && (lhs.getSerial() == rhs.getSerial()));
}

bool Patient::operator==(const Patient* & rhs){
    // since the uniqueness of an object is defined by name and serial number
    // the equality operator considers only those two criteria
    return ((getKey() == rhs->getKey()) && (getSerial() == rhs->getSerial()));
}
