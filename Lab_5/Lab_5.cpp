#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>

using namespace std;

class IAcct {
public:
    virtual int getBalance() const = 0;
    virtual bool withdraw(int amount) = 0;
    virtual void deposit(int amount) = 0;
    virtual ~IAcct() = default;
};

class IObserver {
public:
    virtual void update(IAcct& account, int amount) = 0;
    virtual ~IObserver() = default;
};

class ISubject {
public:
    virtual void attach(IObserver* observer) = 0;
    virtual void detach(IObserver* observer) = 0;
    virtual void notify(int amount) = 0;
    virtual ~ISubject() = default;
};

class Account : public IAcct, public ISubject {
    int balance;
    vector<IObserver*> observers;

public:
    Account(int balance = 0) : balance(max(0, balance)) {}

    int getBalance() const override {
        return balance;
    }

    bool withdraw(int amount) override {
        if (amount <= 0 || amount > balance)
            return false;
        balance -= amount;
        return true;
    }

    void deposit(int amount) override {
        if (amount <= 0)
            return;
        balance += amount;
        notify(amount);
    }

    void attach(IObserver* observer) override {
        if (observer && find(observers.begin(), observers.end(), observer) == observers.end())
            observers.push_back(observer);
    }

    void detach(IObserver* observer) override {
        observers.erase(remove(observers.begin(), observers.end(), observer), observers.end());
    }

    void notify(int amount) override {
        vector<IObserver*> copy = observers;
        for (IObserver* observer : copy) {
            if (find(observers.begin(), observers.end(), observer) != observers.end())
                observer->update(*this, amount);
        }
    }
};

class IDB {
public:
    virtual IAcct* login(const string& accountID) = 0;
    virtual void logout(IAcct*) = 0;
    virtual ~IDB() = default;
};

class AccountDatabase : public IDB {
    map<string, Account> allAccounts;

public:
    void addAccount(const string& accountID, int balance = 0) {
        allAccounts.emplace(accountID, balance);
    }

    IAcct* login(const string& accountID) override {
        auto it = allAccounts.find(accountID);
        if (it == allAccounts.end())
            return nullptr;
        return &it->second;
    }

    void logout(IAcct*) override {
    }
};

class Customer;

class ATMController {
    IDB& database;
    IAcct* myAccount = nullptr;
    Customer* customer = nullptr;

public:
    ATMController(IDB& database) : database(database) {}

    void setCustomer(Customer* customer);
    bool handleLogin(const string& accountID);
    void handleLogout();

    int handleBalanceRequest() const {
        return myAccount ? myAccount->getBalance() : 0;
    }

    bool handleWithdrawal(int amount) {
        return myAccount && myAccount->withdraw(amount);
    }
};

class Customer : public IObserver {
    string name;
    ATMController& atm;
    IAcct* account = nullptr;
    ISubject* subject = nullptr;

protected:
    virtual bool _should_withdraw(int amount) {
        (void)amount;
        return true;
    }

    virtual int _calc_withdraw_amount(int balance) = 0;

    virtual void _after_withdraw(int withdrawn) {
        (void)withdrawn;
    }

public:
    Customer(const string& name, ATMController& atm) : name(name), atm(atm) {}

    void subscribe(IAcct* newAccount) {
        unsubscribe();
        account = newAccount;
        subject = dynamic_cast<ISubject*>(account);
        if (subject)
            subject->attach(this);
    }

    void unsubscribe() {
        if (subject)
            subject->detach(this);
        subject = nullptr;
        account = nullptr;
    }

    void update(IAcct& updatedAccount, int amount) override final {
        if (&updatedAccount != account || !_should_withdraw(amount))
            return;
        int sum = _calc_withdraw_amount(updatedAccount.getBalance());
        if (sum <= 0)
            return;
        bool result = withdraw(sum);
        _after_withdraw(result ? sum : 0);
    }

    bool withdraw(int amount) {
        bool result = atm.handleWithdrawal(amount);
        cout << name << ": " << (result ? "Знято " : "Не вдалося зняти ")
             << amount << " грн" << endl;
        return result;
    }
};

class FullWithdrawalCustomer : public Customer {
protected:
    int _calc_withdraw_amount(int balance) override {
        return balance;
    }

public:
    FullWithdrawalCustomer(const string& name, ATMController& atm)
        : Customer(name, atm) {}
};

class ReserveCustomer : public Customer {
    int reserve;

protected:
    int _calc_withdraw_amount(int balance) override {
        return max(0, balance - reserve);
    }

public:
    ReserveCustomer(const string& name, ATMController& atm, int reserve)
        : Customer(name, atm), reserve(max(0, reserve)) {}
};

void ATMController::setCustomer(Customer* newCustomer) {
    handleLogout();
    customer = newCustomer;
}

bool ATMController::handleLogin(const string& accountID) {
    handleLogout();
    if (!customer)
        return false;
    myAccount = database.login(accountID);
    if (!myAccount)
        return false;
    customer->subscribe(myAccount);
    return true;
}

void ATMController::handleLogout() {
    if (customer)
        customer->unsubscribe();
    if (myAccount)
        database.logout(myAccount);
    myAccount = nullptr;
}

int main() {
    AccountDatabase database;
    database.addAccount("1001");
    database.addAccount("1002");

    ATMController atm1(database);
    ATMController atm2(database);
    FullWithdrawalCustomer customer1("Клієнт 1", atm1);
    ReserveCustomer customer2("Клієнт 2", atm2, 500);

    atm1.setCustomer(&customer1);
    atm2.setCustomer(&customer2);

    atm1.handleLogin("1001");
    atm2.handleLogin("1002");

    IAcct* account1 = database.login("1001");
    IAcct* account2 = database.login("1002");

    account1->deposit(1500);
    cout << "Баланс 1: " << atm1.handleBalanceRequest() << " грн" << endl;

    account2->deposit(1500);
    cout << "Баланс 2: " << atm2.handleBalanceRequest() << " грн" << endl;

    account2->deposit(300);
    cout << "Баланс 2: " << atm2.handleBalanceRequest() << " грн" << endl;

    atm1.handleLogout();
    account1->deposit(700);
    cout << "Баланс 1 після виходу: " << account1->getBalance() << " грн" << endl;

    atm2.handleLogout();
    return 0;
}
