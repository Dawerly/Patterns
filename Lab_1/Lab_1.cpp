#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <cmath>

using namespace std;

class Customer;
class Operator;
class Bill; 

class Customer {
    private:
        int ID;
        string name;
        int age;
        bool isVIP;
        Operator* operators;
        Bill* bills;
    public:
        Customer(int id, const string& n, int a, bool vip);

        void setID(int id){ID = id;};
        void setName(const string& n){name = n;};
        void setAge(int age){this->age = age;};
        void setIsVIP(bool vip){isVIP = vip;};
        void setOperator(Operator* op){operators = op;};
        void setBill(Bill* bill){bills = bill;};

        int getID() const {return ID;};
        void getID(int& id) const {id = ID;};
        void getName(string& n) const {n = name;};
        string getName() const {return name;};
        int getAge() const {return age;};
        void getAge(int& age) const {age = this->age;};
        bool getIsVIP() const {return isVIP;};
        bool getIsVIP(bool& vip) const {vip = isVIP; return isVIP;};
        Operator* getOperator() const {return operators;};
        Bill& getBill() const {return *bills;};
        void Talk(int minutes, Customer& other);
        void Message(int quantity, Customer& other);
        void Internet(int megabytes);
};

class Operator {
    private:
        int ID;
        double messageCost;
        double talkingCharge;
        double networkCharge;
        int discount;
    public:
        Operator(int id, double msgCost, double talkCharge, double netCharge, int Disc);

        void setID(int id){ID = id;};
        void setMessageCost(double msgCost){messageCost = msgCost;};
        void setTalkingCharge(double talkCharge){talkingCharge = talkCharge;};
        void setNetworkCharge(double netCharge){networkCharge = netCharge;};
        void setDiscount(int Disc){discount = Disc;};

        int getID() const {return ID;};
        void getID(int& id) const {id = ID;};
        double getMessageCost() const {return messageCost;};
        void getMessageCost(double& msgCost) const {msgCost = messageCost;};
        double getTalkingCharge() const {return talkingCharge;};
        void getTalkingCharge(double& talkCharge) const {talkCharge = this->talkingCharge;};
        double getNetworkCharge() const {return networkCharge;};
        void getNetworkCharge(double& netCharge) const {netCharge = networkCharge;};
        int getDiscount() const {return discount;};

        double CalculateTalkingCost(int minutes, Customer& customer);
        double CalculateMessageCost(int quantity, Customer& customer, Customer& other);
        double CalculateNetworkCost(int megabytes, Customer& customer);
};

class Bill {
    private:
        double limit;
        double CurentDebt;
    public:
        Bill(double lim);

        void setLimit(double lim){limit = lim;};
        void setCurrentDebt(double debt){CurentDebt = debt;};

        void getLimit(double& lim) const {lim = limit;};
        double getLimit() const {return limit;};
        void getCurrentDebt(double& debt) const {debt = CurentDebt;};
        double getCurrentDebt() const {return CurentDebt;};
        double getLimitingAmount() const {return limit;};

        void Pay(double amount);
        void ChangeLimit(double newLimit);
        void Add(double amount);
        bool CheckLimit(double amount) const;
        void makePayment(double amount){Pay(amount);};
        void changeTheLimit(double newLimit){ChangeLimit(newLimit);};

};

Customer::Customer(int id, const string& n, int a, bool vip)
    : ID(id), name(n), age(a), isVIP(vip), operators(nullptr), bills(nullptr) {}

Operator::Operator(int id, double msgCost, double talkCharge, double netCharge, int Disc)
    : ID(id), messageCost(msgCost), talkingCharge(talkCharge), networkCharge(netCharge), discount(Disc) {}

Bill::Bill(double lim) : limit(lim), CurentDebt(0.0) {}

// ==========================================
void Customer::Talk(int minutes, Customer& other) {
    if(!operators || !bills) return;

    double cost = operators->CalculateTalkingCost(minutes, *this);
    if(bills->CheckLimit(cost)) {
        bills->Add(cost);
    } else {
        cout << "[ERROR] Ліміт перевищено\n";
    }
};

void Customer::Message(int quantity, Customer& other) {
    if(!operators || !bills) return;

    double cost = operators->CalculateMessageCost(quantity, *this, other);
    if(bills->CheckLimit(cost)) {
        bills->Add(cost);
    } else {
        cout << "[ERROR] Ліміт перевищено\n";
    }
};

void Customer::Internet(int megabytes) {
    if(!operators || !bills) return;

    double cost = operators->CalculateNetworkCost(megabytes, *this);
    if(bills->CheckLimit(cost)) {
        bills->Add(cost);
    } else {
        cout << "[ERROR] Ліміт перевищено\n";
    }
};

// ==========================================


// ==========================================
double Operator::CalculateTalkingCost(int minutes, Customer& customer) {
    double cost = talkingCharge * minutes;

    if(customer.getAge() < 18 || customer.getAge() > 60) {
        cost *= (1.0 - discount / 100.0);
    };
    if(customer.getIsVIP()) {
        cost *= (1.0 - discount / 100.0);
    };

    return cost;
};

double Operator::CalculateMessageCost(int quantity, Customer& customer, Customer& other) {
    double cost = messageCost * quantity;

    if (customer.getOperator() != nullptr && other.getOperator() != nullptr &&
        customer.getOperator()->getID() == other.getOperator()->getID()) {
        cost *= (1.0 - discount / 100.0);
    }

    if(customer.getIsVIP()) {
        cost *= (1.0 - discount / 100.0);
    };
    
    return cost;
};

double Operator::CalculateNetworkCost(int megabytes, Customer& customer) {
    double cost = networkCharge * megabytes;
    
    if(customer.getIsVIP()) {
        cost *= (1.0 - discount / 100.0);
    };

    return cost;
};

// ==========================================


// ==========================================
void Bill::Pay(double amount) {
    CurentDebt -= amount;
};

void Bill::ChangeLimit(double newLimit) {
    limit = newLimit;
};

void Bill::Add(double amount){
    CurentDebt += amount;
};

bool Bill::CheckLimit(double amount) const{
    return (CurentDebt + amount) <= limit;
};
// ==========================================


int main() {

    int N, M;

    while (true) {

        cout << "Введіть кількість клієнтів (N): ";
        cin >> N;

        cout << "Введіть кількість операторів (M): ";
        cin >> M;

        if (N <= 0 || M <= 0) {
            cout << "Кількість клієнтів та операторів повинна бути додатною!\n\n";
        continue;
        }
        if (N > 100 || M > 100) {
            cout << "Кількість клієнтів та операторів не повинна перевищувати 100!\n\n";
            continue;
        }
        if (N < 2 || M < 1) {
            cout << "Для роботи з клієнтами потрібно принаймні 2 клієнти та 1 оператор!\n\n";
            continue;
        }
        break;
    }
    cout << "----------------------------" << endl;

    vector<Operator*> operators(M);
    vector<Customer*> customers(N);
    vector<Bill*> bills(N);

    for(int i = 0; i < M; ++i){
        int ID;
        double messageCost, talkingCharge, networkCharge;
        int discount;

        cout << "Введіть дані для оператора #" << (i + 1) << ":\n";
        cout << "ID: ";
        cin >> ID;
        cout << "Ціна повідомлення: ";
        cin >> messageCost;
        cout << "Ціна розмови: ";
        cin >> talkingCharge;
        cout << "Ціна інтернету: ";
        cin >> networkCharge;
        cout << "Знижка (%): ";
        cin >> discount;

        operators[i] = new Operator(ID, messageCost, talkingCharge, networkCharge, discount);
        cout << "----------------------------" << endl;
    };

    for(int i = 0; i < N; ++i){
        int ID, age;
        string name;
        bool isVIP;
        double limit;

        cout << "\n----------------------------" << endl;
        cout << "Введіть дані для клієнта #" << (i + 1) << ":\n";
        cout << "ID: ";
        cin >> ID;
        cout << "Ім'я: ";
        cin >> name;
        cout << "Вік: ";
        cin >> age;
        cout << "Ліміт рахунку: ";
        cin >> limit;
        cout << "VIP (1 - так, 0 - ні): ";
        cin >> isVIP;   
        cout << "Виберіть оператора (1-" << M << "): ";
        int operatorIndex;
        cin >> operatorIndex;
        while(operatorIndex < 1 || operatorIndex > M){
            cout << "Невірний вибір оператора!\n";
            cout << "Виберіть оператора (1-" << M << "): ";
            cin >> operatorIndex;
        }

        bills[i] = new Bill(limit);
        customers[i] = new Customer(ID, name, age, isVIP);
        customers[i]->setOperator(operators[operatorIndex - 1]);
        customers[i]->setBill(bills[i]);
        cout << "----------------------------" << endl;        
    };

    cout << "\n=== Дані введено успішно! ===\n";

    cout << "\n=== Симуляція дій клієнтів ===\n";
    for(int i = 0; i < N; ++i) {
        cout << "\n----------------------------" << endl;
        cout << "Клієнт " << customers[i]->getName() << endl;
        cout << "Бажаєте здійснити дзвінок? (1 - так, 0 - ні): ";
        int callChoice;
        cin >> callChoice;
        if(callChoice == 1) {
            cout << "Введіть тривалість дзвінка (хв): ";
            int minutes;
            cin >> minutes;
            cout << "Введіть ID іншого клієнта для дзвінка: ";
            int otherID;
            cin >> otherID;
            while(otherID < 0 || otherID > N-1 || otherID == i) {
                cout << "Невірний ID, спробуйте ще раз\n";
                cin >> otherID;
            }
            customers[i]->Talk(minutes, *customers[otherID]);
            cout << "Виконується дзвінок на " << minutes << " хвилин до клієнта " << customers[otherID]->getName() << endl;
            cout << "Вартість дзвінка: " << customers[i]->getOperator()->CalculateTalkingCost(minutes, *customers[i]) << " грн\n";
            cout << "----------------------------" << endl;
        }
        else {
            cout << "\n";
        }

        cout << "Бажаєте надіслати повідомлення? (1 - так, 0 - ні): ";
        int messageChoice;
        cin >> messageChoice;
        if(messageChoice == 1) {
            cout << "Введіть кількість повідомлень: ";
            int quantity;
            cin >> quantity;
            cout << "Введіть ID клієнта для повідомлення: ";
            int otherID;
            cin >> otherID;
            if(otherID < 0 || otherID > N-1 || otherID == i) {
                cout << "Невірний ID, спробуйте ще раз\n";
                while(otherID < 0 || otherID > N-1 || otherID == i) {
                    cout << "Введіть коректний ID: ";
                    cin >> otherID;
                }
            }
            cout << "Виконується надсилання повідомлення до клієнта " << customers[otherID]->getName() << endl;
            customers[i]->Message(quantity, *customers[otherID]);
            cout << "Вартість повідомлення: " << customers[i]->getOperator()->CalculateMessageCost(quantity, *customers[i], *customers[otherID]) << " грн\n";
        cout << "----------------------------" << endl;
        } 
        else {
            cout << "\n";
        }

        cout << "Бажаєте використати інтернет? (1 - так, 0 - ні): ";
        int internetChoice;
        cin >> internetChoice;
        if(internetChoice == 1) {
            cout << "Введіть кількість мегабайтів: ";
            int megabytes;
            cin >> megabytes;
            cout << "Виконується використання інтернету на " << megabytes << " мегабайтів\n";
            customers[i]->Internet(megabytes);
            cout << "Вартість інтернету: " << customers[i]->getOperator()->CalculateNetworkCost(megabytes, *customers[i]) << " грн\n";
            cout << "----------------------------" << endl;
        }
        else {
            cout << "\n";
        }

        cout << "Поточний борг клієнта " << customers[i]->getName() << ": " << customers[i]->getBill().getCurrentDebt() << " грн\n";
        cout << "Бажаєте оплатити борг? (1 - так, 0 - ні): ";
        int paymentChoice;
        cin >> paymentChoice;
        if(paymentChoice == 1) {
            cout << "Введіть суму оплати: ";
            double paymentAmount;
            cin >> paymentAmount;
            customers[i]->getBill().makePayment(paymentAmount);
            cout << "----------------------------" << endl;
        } 
        else {
            cout << "\n";
        }

        cout << "Поточний ліміт клієнта " << customers[i]->getName() << ": " << customers[i]->getBill().getLimitingAmount() << " грн\n";
        cout << "Бажаєте змінити ліміт? (1 - так, 0 - ні): ";
        int limitChoice;
        cin >> limitChoice;
        if(limitChoice == 1) {
            cout << "Введіть новий ліміт: ";
            double newLimit;
            cin >> newLimit;
            customers[i]->getBill().changeTheLimit(newLimit);
            cout << "----------------------------" << endl;
        } 
        else {
            cout << "\n";
        }
    };

    for (int i = 0; i < N; ++i) {
        delete customers[i];
        delete bills[i];
    }

    for (int i = 0; i < M; ++i) {
        delete operators[i];
    }

    return 0;

};