#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <iomanip>

using namespace std;

class Port;
class Ship;
class Container;

class IPort {
    public:
        virtual void incomingShip(Ship* ship) = 0;
        virtual void outgoingShip(Ship* ship) = 0;
        virtual ~IPort() {}
};

class IShip {
    public:
        virtual bool sailTo(Port* port) = 0;
        virtual void reFuel(double newFuel) = 0;
        virtual bool load(Container* container) = 0;
        virtual bool unLoad(Container* container) = 0;
        virtual ~IShip() {}
};

class Container {
    protected:
        int ID;
        int weight;

    public:
        Container(int id, int w);

        int getID() const {return ID;};
        int getWeight() const {return weight;};

        virtual double consumption() const = 0;
        virtual string getType() const = 0;
        virtual ~Container() {}

        bool equals(Container* other) const;
};

class BasicContainer : public Container {
    public:
        BasicContainer(int id, int weight);
        double consumption() const override;
        string getType() const override {return "BasicContainer";};
};

class HeavyContainer : public Container {
    public:
        HeavyContainer(int id, int weight);
        double consumption() const override;
        string getType() const override {return "HeavyContainer";};
};

class RefrigeratedContainer : public HeavyContainer {
    public:
        RefrigeratedContainer(int id, int weight);
        double consumption() const override;
        string getType() const override {return "RefrigeratedContainer";};
};

class LiquidContainer : public HeavyContainer {
    public:
        LiquidContainer(int id, int weight);
        double consumption() const override;
        string getType() const override {return "LiquidContainer";};
};

class Port : public IPort {
    private:
        int ID;
        double latitude;
        double longitude;
        vector<Container*> containers;
        vector<Ship*> history;
        vector<Ship*> current;

    public:
        Port(int id, double lat, double lon);

        int getID() const {return ID;};
        double getLatitude() const {return latitude;};
        double getLongitude() const {return longitude;};
        vector<Container*>& getContainers() {return containers;};
        vector<Ship*>& getCurrentShips() {return current;};
        vector<Ship*>& getHistory() {return history;};

        double getDistance(Port* other) const;
        void addContainer(Container* container);
        bool removeContainer(Container* container);
        bool hasContainer(Container* container) const;

        void incomingShip(Ship* ship) override;
        void outgoingShip(Ship* ship) override;
};

class Ship : public IShip {
    private:
        int ID;
        double fuel;
        Port* currentPort;
        int totalWeightCapacity;
        int maxNumberOfAllContainers;
        int maxNumberOfHeavyContainers;
        int maxNumberOfRefrigeratedContainers;
        int maxNumberOfLiquidContainers;
        double fuelConsumptionPerKM;
        vector<Container*> containers;

    public:
        Ship(int id, Port* port, int weightCapacity, int allContainers,
             int heavyContainers, int refrigeratedContainers,
             int liquidContainers, double fuelPerKM);

        int getID() const {return ID;};
        double getFuel() const {return fuel;};
        Port* getCurrentPort() const {return currentPort;};
        vector<Container*> getCurrentContainers() const;

        int getCurrentWeight() const;
        double getRequiredFuel(Port* port) const;

        bool sailTo(Port* port) override;
        void reFuel(double newFuel) override;
        bool load(Container* container) override;
        bool unLoad(Container* container) override;
};

Container::Container(int id, int w) : ID(id), weight(w) {}

bool Container::equals(Container* other) const {
    if(other == nullptr) return false;

    return ID == other->getID() &&
           weight == other->getWeight() &&
           getType() == other->getType();
};

BasicContainer::BasicContainer(int id, int weight)
    : Container(id, weight) {}

double BasicContainer::consumption() const {
    return weight * 2.50;
};

HeavyContainer::HeavyContainer(int id, int weight)
    : Container(id, weight) {}

double HeavyContainer::consumption() const {
    return weight * 3.00;
};

RefrigeratedContainer::RefrigeratedContainer(int id, int weight)
    : HeavyContainer(id, weight) {}

double RefrigeratedContainer::consumption() const {
    return weight * 5.00;
};

LiquidContainer::LiquidContainer(int id, int weight)
    : HeavyContainer(id, weight) {}

double LiquidContainer::consumption() const {
    return weight * 4.00;
};

Port::Port(int id, double lat, double lon)
    : ID(id), latitude(lat), longitude(lon) {}

double Port::getDistance(Port* other) const {
    double x = latitude - other->getLatitude();
    double y = longitude - other->getLongitude();

    return sqrt(x * x + y * y);
};

void Port::addContainer(Container* container) {
    if(!hasContainer(container)) {
        containers.push_back(container);
    }
};

bool Port::removeContainer(Container* container) {
    for(int i = 0; i < (int)containers.size(); ++i) {
        if(containers[i] == container) {
            containers.erase(containers.begin() + i);
            return true;
        }
    }

    return false;
};

bool Port::hasContainer(Container* container) const {
    for(Container* currentContainer : containers) {
        if(currentContainer == container) {
            return true;
        }
    }

    return false;
};

void Port::incomingShip(Ship* ship) {
    for(Ship* currentShip : current) {
        if(currentShip == ship) return;
    }

    current.push_back(ship);
};

void Port::outgoingShip(Ship* ship) {
    for(int i = 0; i < (int)current.size(); ++i) {
        if(current[i] == ship) {
            current.erase(current.begin() + i);
            break;
        }
    }

    for(Ship* historyShip : history) {
        if(historyShip == ship) return;
    }

    history.push_back(ship);
};

Ship::Ship(int id, Port* port, int weightCapacity, int allContainers,
           int heavyContainers, int refrigeratedContainers,
           int liquidContainers, double fuelPerKM)
    : ID(id), fuel(0), currentPort(port),
      totalWeightCapacity(weightCapacity),
      maxNumberOfAllContainers(allContainers),
      maxNumberOfHeavyContainers(heavyContainers),
      maxNumberOfRefrigeratedContainers(refrigeratedContainers),
      maxNumberOfLiquidContainers(liquidContainers),
      fuelConsumptionPerKM(fuelPerKM) {

    currentPort->incomingShip(this);
};

int Ship::getCurrentWeight() const {
    int totalWeight = 0;

    for(Container* container : containers) {
        totalWeight += container->getWeight();
    }

    return totalWeight;
};

vector<Container*> Ship::getCurrentContainers() const {
    vector<Container*> sortedContainers = containers;

    sort(sortedContainers.begin(), sortedContainers.end(),
        [](Container* first, Container* second) {
            return first->getID() < second->getID();
        });

    return sortedContainers;
};

double Ship::getRequiredFuel(Port* port) const {
    double totalConsumption = fuelConsumptionPerKM;

    for(Container* container : containers) {
        totalConsumption += container->consumption();
    }

    return currentPort->getDistance(port) * totalConsumption;
};

void Ship::reFuel(double newFuel) {
    if(newFuel > 0) {
        fuel += newFuel;
    }
};

bool Ship::load(Container* container) {
    if(container == nullptr || !currentPort->hasContainer(container)) {
        return false;
    }

    if((int)containers.size() >= maxNumberOfAllContainers) {
        return false;
    }

    if(getCurrentWeight() + container->getWeight() > totalWeightCapacity) {
        return false;
    }

    int heavyCount = 0;
    int refrigeratedCount = 0;
    int liquidCount = 0;

    for(Container* currentContainer : containers) {
        if(currentContainer->getType() == "HeavyContainer" ||
           currentContainer->getType() == "RefrigeratedContainer" ||
           currentContainer->getType() == "LiquidContainer") {
            heavyCount++;
        }

        if(currentContainer->getType() == "RefrigeratedContainer") {
            refrigeratedCount++;
        }

        if(currentContainer->getType() == "LiquidContainer") {
            liquidCount++;
        }
    }

    if(container->getType() == "HeavyContainer" ||
       container->getType() == "RefrigeratedContainer" ||
       container->getType() == "LiquidContainer") {
        if(heavyCount >= maxNumberOfHeavyContainers) return false;
    }

    if(container->getType() == "RefrigeratedContainer" &&
       refrigeratedCount >= maxNumberOfRefrigeratedContainers) {
        return false;
    }

    if(container->getType() == "LiquidContainer" &&
       liquidCount >= maxNumberOfLiquidContainers) {
        return false;
    }

    containers.push_back(container);
    currentPort->removeContainer(container);

    return true;
};

bool Ship::unLoad(Container* container) {
    for(int i = 0; i < (int)containers.size(); ++i) {
        if(containers[i] == container) {
            currentPort->addContainer(container);
            containers.erase(containers.begin() + i);
            return true;
        }
    }

    return false;
};

bool Ship::sailTo(Port* port) {
    if(port == nullptr || port == currentPort) {
        return false;
    }

    double requiredFuel = getRequiredFuel(port);

    if(fuel < requiredFuel) {
        return false;
    }

    fuel -= requiredFuel;
    currentPort->outgoingShip(this);
    port->incomingShip(this);
    currentPort = port;

    return true;
};

Port* findPort(vector<Port*>& ports, int id) {
    for(Port* port : ports) {
        if(port->getID() == id) return port;
    }

    return nullptr;
};

Ship* findShip(vector<Ship*>& ships, int id) {
    for(Ship* ship : ships) {
        if(ship->getID() == id) return ship;
    }

    return nullptr;
};

Container* findContainer(vector<Container*>& containers, int id) {
    for(Container* container : containers) {
        if(container->getID() == id) return container;
    }

    return nullptr;
};

void printContainerIDs(vector<Container*> containers, const string& type) {
    vector<int> IDs;

    for(Container* container : containers) {
        if(container->getType() == type) {
            IDs.push_back(container->getID());
        }
    }

    sort(IDs.begin(), IDs.end());

    cout << "[";
    for(int i = 0; i < (int)IDs.size(); ++i) {
        cout << IDs[i];
        if(i < (int)IDs.size() - 1) cout << ", ";
    }
    cout << "]";
};

void printState(vector<Port*>& ports) {
    sort(ports.begin(), ports.end(),
        [](Port* first, Port* second) {
            return first->getID() < second->getID();
        });

    cout << fixed << setprecision(2);
    cout << "\n========== СТАН СИСТЕМИ ==========\n";

    if(ports.empty()) {
        cout << "Портів ще немає.\n";
        return;
    }

    for(Port* port : ports) {
        cout << "\nPort " << port->getID() << ": ("
             << port->getLatitude() << ", "
             << port->getLongitude() << ")\n";

        cout << "BasicContainer: ";
        printContainerIDs(port->getContainers(), "BasicContainer");
        cout << endl;

        cout << "HeavyContainer: ";
        printContainerIDs(port->getContainers(), "HeavyContainer");
        cout << endl;

        cout << "RefrigeratedContainer: ";
        printContainerIDs(port->getContainers(), "RefrigeratedContainer");
        cout << endl;

        cout << "LiquidContainer: ";
        printContainerIDs(port->getContainers(), "LiquidContainer");
        cout << endl;

        vector<Ship*> portShips = port->getCurrentShips();

        sort(portShips.begin(), portShips.end(),
            [](Ship* first, Ship* second) {
                return first->getID() < second->getID();
            });

        for(Ship* ship : portShips) {
            cout << "  Ship " << ship->getID()
                 << ": fuel = " << ship->getFuel() << endl;

            cout << "    BasicContainer: ";
            printContainerIDs(ship->getCurrentContainers(), "BasicContainer");
            cout << endl;

            cout << "    HeavyContainer: ";
            printContainerIDs(ship->getCurrentContainers(), "HeavyContainer");
            cout << endl;

            cout << "    RefrigeratedContainer: ";
            printContainerIDs(ship->getCurrentContainers(), "RefrigeratedContainer");
            cout << endl;

            cout << "    LiquidContainer: ";
            printContainerIDs(ship->getCurrentContainers(), "LiquidContainer");
            cout << endl;
        }
    }
};

void printMenu() {
    cout << "\n========== МЕНЮ ==========\n";
    cout << "1. Створити порт\n";
    cout << "2. Створити контейнер\n";
    cout << "3. Створити корабель\n";
    cout << "4. Завантажити контейнер\n";
    cout << "5. Вивантажити контейнер\n";
    cout << "6. Відправити корабель до іншого порту\n";
    cout << "7. Заправити корабель\n";
    cout << "8. Показати стан системи\n";
    cout << "0. Завершити програму\n";
};

// Уся робота програми знаходиться в одній функції
void runProgram() {
    vector<Port*> ports;
    vector<Ship*> ships;
    vector<Container*> containers;

    int choice;

    do {
        printMenu();
        cout << "Ваш вибір: ";
        cin >> choice;

        if(choice == 1) {
            double latitude, longitude;

            cout << "Введіть latitude: ";
            cin >> latitude;
            cout << "Введіть longitude: ";
            cin >> longitude;

            int ID = ports.size();
            ports.push_back(new Port(ID, latitude, longitude));

            cout << "Порт створено. Його ID: " << ID << endl;
        }

        else if(choice == 2) {
            if(ports.empty()) {
                cout << "Спочатку потрібно створити порт!\n";
                continue;
            }

            int portID, weight;
            char type;

            cout << "Введіть ID порту: ";
            cin >> portID;

            Port* port = findPort(ports, portID);

            if(port == nullptr) {
                cout << "Порт з таким ID не знайдено!\n";
                continue;
            }

            cout << "Введіть вагу контейнера: ";
            cin >> weight;

            if(weight <= 0) {
                cout << "Вага повинна бути додатною!\n";
                continue;
            }

            cout << "Тип контейнера (N - звичайний, R - холодильний, L - рідинний): ";
            cin >> type;

            int ID = containers.size();
            Container* newContainer = nullptr;

            if(type == 'R' || type == 'r') {
                newContainer = new RefrigeratedContainer(ID, weight);
            }
            else if(type == 'L' || type == 'l') {
                newContainer = new LiquidContainer(ID, weight);
            }
            else if(type == 'N' || type == 'n') {
                if(weight <= 3000) {
                    newContainer = new BasicContainer(ID, weight);
                }
                else {
                    newContainer = new HeavyContainer(ID, weight);
                }
            }
            else {
                cout << "Невірний тип контейнера!\n";
                continue;
            }

            containers.push_back(newContainer);
            port->addContainer(newContainer);

            cout << newContainer->getType()
                 << " створено. Його ID: " << ID << endl;
        }

        else if(choice == 3) {
            if(ports.empty()) {
                cout << "Спочатку потрібно створити порт!\n";
                continue;
            }

            int portID;
            int totalWeightCapacity;
            int maxNumberOfAllContainers;
            int maxNumberOfHeavyContainers;
            int maxNumberOfRefrigeratedContainers;
            int maxNumberOfLiquidContainers;
            double fuelConsumptionPerKM;

            cout << "Введіть ID початкового порту: ";
            cin >> portID;

            Port* port = findPort(ports, portID);

            if(port == nullptr) {
                cout << "Порт з таким ID не знайдено!\n";
                continue;
            }

            cout << "Максимальна загальна вага: ";
            cin >> totalWeightCapacity;
            cout << "Максимальна кількість усіх контейнерів: ";
            cin >> maxNumberOfAllContainers;
            cout << "Максимальна кількість важких контейнерів: ";
            cin >> maxNumberOfHeavyContainers;
            cout << "Максимальна кількість холодильних контейнерів: ";
            cin >> maxNumberOfRefrigeratedContainers;
            cout << "Максимальна кількість рідинних контейнерів: ";
            cin >> maxNumberOfLiquidContainers;
            cout << "Витрата пального корабля на 1 км: ";
            cin >> fuelConsumptionPerKM;

            if(totalWeightCapacity <= 0 || maxNumberOfAllContainers < 0 ||
               maxNumberOfHeavyContainers < 0 ||
               maxNumberOfRefrigeratedContainers < 0 ||
               maxNumberOfLiquidContainers < 0 ||
               fuelConsumptionPerKM <= 0) {
                cout << "Дані корабля введено неправильно!\n";
                continue;
            }

            int ID = ships.size();

            ships.push_back(new Ship(
                ID, port, totalWeightCapacity,
                maxNumberOfAllContainers,
                maxNumberOfHeavyContainers,
                maxNumberOfRefrigeratedContainers,
                maxNumberOfLiquidContainers,
                fuelConsumptionPerKM
            ));

            cout << "Корабель створено. Його ID: " << ID << endl;
        }

        else if(choice == 4) {
            int shipID, containerID;

            cout << "Введіть ID корабля: ";
            cin >> shipID;
            cout << "Введіть ID контейнера: ";
            cin >> containerID;

            Ship* ship = findShip(ships, shipID);
            Container* container = findContainer(containers, containerID);

            if(ship == nullptr || container == nullptr) {
                cout << "Корабель або контейнер не знайдено!\n";
            }
            else if(ship->load(container)) {
                cout << "Контейнер успішно завантажено.\n";
            }
            else {
                cout << "Контейнер неможливо завантажити. Перевірте порт та обмеження корабля.\n";
            }
        }

        else if(choice == 5) {
            int shipID, containerID;

            cout << "Введіть ID корабля: ";
            cin >> shipID;
            cout << "Введіть ID контейнера: ";
            cin >> containerID;

            Ship* ship = findShip(ships, shipID);
            Container* container = findContainer(containers, containerID);

            if(ship == nullptr || container == nullptr) {
                cout << "Корабель або контейнер не знайдено!\n";
            }
            else if(ship->unLoad(container)) {
                cout << "Контейнер успішно вивантажено.\n";
            }
            else {
                cout << "Цього контейнера немає на кораблі.\n";
            }
        }

        else if(choice == 6) {
            int shipID, portID;

            cout << "Введіть ID корабля: ";
            cin >> shipID;
            cout << "Введіть ID порту призначення: ";
            cin >> portID;

            Ship* ship = findShip(ships, shipID);
            Port* port = findPort(ports, portID);

            if(ship == nullptr || port == nullptr) {
                cout << "Корабель або порт не знайдено!\n";
            }
            else {
                double requiredFuel = ship->getRequiredFuel(port);

                cout << fixed << setprecision(2);
                cout << "Для подорожі потрібно пального: "
                     << requiredFuel << endl;

                if(ship->sailTo(port)) {
                    cout << "Корабель успішно прибув до порту "
                         << portID << ".\n";
                }
                else {
                    cout << "Корабель не може відправитися. Недостатньо пального або вибрано поточний порт.\n";
                }
            }
        }

        else if(choice == 7) {
            int shipID;
            double fuel;

            cout << "Введіть ID корабля: ";
            cin >> shipID;

            Ship* ship = findShip(ships, shipID);

            if(ship == nullptr) {
                cout << "Корабель з таким ID не знайдено!\n";
                continue;
            }

            cout << "Введіть кількість пального: ";
            cin >> fuel;

            if(fuel <= 0) {
                cout << "Кількість пального повинна бути додатною!\n";
                continue;
            }

            ship->reFuel(fuel);
            cout << "Корабель заправлено. Пального зараз: "
                 << fixed << setprecision(2)
                 << ship->getFuel() << endl;
        }

        else if(choice == 8) {
            printState(ports);
        }

        else if(choice != 0) {
            cout << "Невірний пункт меню!\n";
        }

    } while(choice != 0);

    for(Container* container : containers) {
        delete container;
    }

    for(Ship* ship : ships) {
        delete ship;
    }

    for(Port* port : ports) {
        delete port;
    }

    cout << "Програму завершено.\n";
};

int main() {
    runProgram();
    return 0;
};
