#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <stdexcept>
#include <iomanip>
#include <cctype>

using namespace std;

class Database;

class DataType {
    public:
        virtual bool validate(const string& value) const = 0;
        virtual string getName() const = 0;
        virtual ~DataType() {}
};

class IntegerType : public DataType {
    public:
        bool validate(const string& value) const override {
            if(value.empty()) return false;

            int start = (value[0] == '-') ? 1 : 0;
            if(start == 1 && value.size() == 1) return false;

            for(int i = start; i < (int)value.size(); ++i) {
                if(!isdigit((unsigned char)value[i])) return false;
            }

            return true;
        };

        string getName() const override {return "Integer";};
};

class StringType : public DataType {
    private:
        int maxLength;

    public:
        StringType(int length = 100) : maxLength(length) {}

        bool validate(const string& value) const override {
            return (int)value.size() <= maxLength;
        };

        string getName() const override {return "String";};
};

class BooleanType : public DataType {
    public:
        bool validate(const string& value) const override {
            return value == "true" || value == "false" ||
                   value == "1" || value == "0";
        };

        string getName() const override {return "Boolean";};
};

class DateType : public DataType {
    public:
        bool validate(const string& value) const override {
            if(value.size() != 10 || value[4] != '-' || value[7] != '-') {
                return false;
            }

            for(int i = 0; i < 10; ++i) {
                if(i != 4 && i != 7 && !isdigit((unsigned char)value[i])) {
                    return false;
                }
            }

            int year = stoi(value.substr(0, 4));
            int month = stoi(value.substr(5, 2));
            int day = stoi(value.substr(8, 2));
            int days[] = {31, 28, 31, 30, 31, 30,
                          31, 31, 30, 31, 30, 31};

            if(year % 400 == 0 || (year % 4 == 0 && year % 100 != 0)) {
                days[1] = 29;
            }

            return year > 0 && month >= 1 && month <= 12 &&
                   day >= 1 && day <= days[month - 1];
        };

        string getName() const override {return "Date";};
};

class Column {
    private:
        string name;
        DataType* type;
        bool nullable;
        bool primaryKey;
        bool foreignKey;
        string foreignTable;
        string foreignColumn;

    public:
        Column(const string& n, DataType* t, bool nullValue = true,
               bool primary = false, bool foreign = false,
               const string& table = "", const string& column = "")
            : name(n), type(t), nullable(nullValue), primaryKey(primary),
              foreignKey(foreign), foreignTable(table),
              foreignColumn(column) {
            if(primaryKey) nullable = false;
        };

        ~Column() {delete type;};

        string getName() const {return name;};
        DataType* getType() const {return type;};
        bool isNullable() const {return nullable;};
        bool isPrimaryKey() const {return primaryKey;};
        bool isForeignKey() const {return foreignKey;};
        string getForeignTable() const {return foreignTable;};
        string getForeignColumn() const {return foreignColumn;};

        bool validate(const string& value) const {
            if(value.empty()) return nullable;
            return type->validate(value);
        };
};

class Row {
    private:
        int ID;
        map<string, string> data;

    public:
        Row(int id, const map<string, string>& values)
            : ID(id), data(values) {}

        int getID() const {return ID;};

        string getValue(const string& column) const {
            auto element = data.find(column);
            return element == data.end() ? "" : element->second;
        };

        void setValue(const string& column, const string& value) {
            data[column] = value;
        };
};

class Table {
    private:
        string name;
        vector<Column*> columns;
        vector<Row*> rows;
        int nextID;

    public:
        Table(const string& n, const vector<Column*>& cols)
            : name(n), columns(cols), nextID(1) {}

        ~Table() {
            for(Row* row : rows) delete row;
            for(Column* column : columns) delete column;
        };

        string getName() const {return name;};
        vector<Column*>& getColumns() {return columns;};
        vector<Row*>& getRows() {return rows;};

        Column* getColumn(const string& columnName) const {
            for(Column* column : columns) {
                if(column->getName() == columnName) return column;
            }
            return nullptr;
        };

        Row* getRow(int rowID) const {
            for(Row* row : rows) {
                if(row->getID() == rowID) return row;
            }
            return nullptr;
        };

        bool hasValue(const string& column, const string& value,
                      int ignoredID = -1) const {
            for(Row* row : rows) {
                if(row->getID() != ignoredID &&
                   row->getValue(column) == value) return true;
            }
            return false;
        };

        Row* insert(const map<string, string>& data) {
            map<string, string> fullData;

            for(Column* column : columns) {
                auto element = data.find(column->getName());
                string value = element == data.end() ? "" : element->second;

                if(!column->validate(value)) {
                    throw invalid_argument(
                        "Неправильне значення колонки " + column->getName()
                    );
                }

                if(column->isPrimaryKey() &&
                   hasValue(column->getName(), value)) {
                    throw invalid_argument("Primary key повинен бути унікальним");
                }

                fullData[column->getName()] = value;
            }

            Row* row = new Row(nextID++, fullData);
            rows.push_back(row);
            return row;
        };

        bool update(int rowID, const string& columnName,
                    const string& value) {
            Row* row = getRow(rowID);
            Column* column = getColumn(columnName);

            if(row == nullptr || column == nullptr ||
               !column->validate(value)) return false;

            if(column->isPrimaryKey() &&
               hasValue(columnName, value, rowID)) return false;

            row->setValue(columnName, value);
            return true;
        };

        bool remove(int rowID) {
            for(int i = 0; i < (int)rows.size(); ++i) {
                if(rows[i]->getID() == rowID) {
                    delete rows[i];
                    rows.erase(rows.begin() + i);
                    return true;
                }
            }
            return false;
        };

        void print() const {
            cout << "\nТаблиця " << name << endl;
            cout << "row_id";
            for(Column* column : columns) cout << " | " << column->getName();
            cout << "\n--------------------------------------------------\n";

            for(Row* row : rows) {
                cout << row->getID();
                for(Column* column : columns) {
                    string value = row->getValue(column->getName());
                    cout << " | " << (value.empty() ? "NULL" : value);
                }
                cout << endl;
            }
        };
};

class SimpleQuery {
    private:
        Table* table;
        vector<string> selectedColumns;
        string whereColumn;
        string whereOperator;
        string whereValue;
        string sortColumn;
        bool ascending;

        bool compare(const string& left, const string& right,
                     const string& operation, Column* column) const {
            if(operation == "=") return left == right;
            if(operation == "!=") return left != right;

            if(column->getType()->getName() == "Integer") {
                long long first = stoll(left);
                long long second = stoll(right);
                if(operation == ">") return first > second;
                if(operation == "<") return first < second;
                if(operation == ">=") return first >= second;
                if(operation == "<=") return first <= second;
            }
            else {
                if(operation == ">") return left > right;
                if(operation == "<") return left < right;
                if(operation == ">=") return left >= right;
                if(operation == "<=") return left <= right;
            }

            throw invalid_argument("Невідомий оператор запиту");
        };

    public:
        SimpleQuery(Table* selectedTable)
            : table(selectedTable), ascending(true) {}

        SimpleQuery& select(const vector<string>& columns) {
            selectedColumns = columns;
            return *this;
        };

        SimpleQuery& where(const string& column, const string& operation,
                           const string& value) {
            if(table->getColumn(column) == nullptr) {
                throw invalid_argument("Колонку WHERE не знайдено");
            }

            whereColumn = column;
            whereOperator = operation;
            whereValue = value;
            return *this;
        };

        SimpleQuery& sortBy(const string& column, bool isAscending = true) {
            if(table->getColumn(column) == nullptr) {
                throw invalid_argument("Колонку сортування не знайдено");
            }

            sortColumn = column;
            ascending = isAscending;
            return *this;
        };

        vector<Row*> execute() const {
            vector<Row*> result;

            for(Row* row : table->getRows()) {
                if(whereColumn.empty() ||
                   compare(row->getValue(whereColumn), whereValue,
                           whereOperator, table->getColumn(whereColumn))) {
                    result.push_back(row);
                }
            }

            if(!sortColumn.empty()) {
                Column* column = table->getColumn(sortColumn);

                sort(result.begin(), result.end(),
                    [this, column](Row* firstRow, Row* secondRow) {
                        string first = firstRow->getValue(sortColumn);
                        string second = secondRow->getValue(sortColumn);

                        if(column->getType()->getName() == "Integer") {
                            if(ascending) return stoll(first) < stoll(second);
                            return stoll(first) > stoll(second);
                        }

                        if(ascending) return first < second;
                        return first > second;
                    });
            }

            return result;
        };

        void print() const {
            vector<string> columns = selectedColumns;
            if(columns.empty()) {
                for(Column* column : table->getColumns()) {
                    columns.push_back(column->getName());
                }
            }

            cout << "\nРезультат SimpleQuery\n";
            for(string column : columns) cout << column << " | ";
            cout << "\n--------------------------------------------------\n";

            for(Row* row : execute()) {
                for(string column : columns) {
                    cout << row->getValue(column) << " | ";
                }
                cout << endl;
            }
        };

        int count() const {return execute().size();};

        long long sum(const string& column) const {
            if(table->getColumn(column) == nullptr ||
               table->getColumn(column)->getType()->getName() != "Integer") {
                throw invalid_argument("SUM працює тільки з Integer");
            }

            long long result = 0;
            for(Row* row : execute()) result += stoll(row->getValue(column));
            return result;
        };

        double average(const string& column) const {
            int amount = count();
            return amount == 0 ? 0 : (double)sum(column) / amount;
        };
};

class JoinedTable {
    private:
        vector<map<string, string>> rows;

    public:
        JoinedTable(Table* firstTable, Table* secondTable,
                    const string& firstColumn,
                    const string& secondColumn) {
            for(Row* firstRow : firstTable->getRows()) {
                for(Row* secondRow : secondTable->getRows()) {
                    if(firstRow->getValue(firstColumn) ==
                       secondRow->getValue(secondColumn)) {
                        map<string, string> newRow;

                        for(Column* column : firstTable->getColumns()) {
                            newRow[firstTable->getName() + "." +
                                   column->getName()] =
                                firstRow->getValue(column->getName());
                        }

                        for(Column* column : secondTable->getColumns()) {
                            newRow[secondTable->getName() + "." +
                                   column->getName()] =
                                secondRow->getValue(column->getName());
                        }

                        rows.push_back(newRow);
                    }
                }
            }
        };

        void print() const {
            cout << "\nРезультат INNER JOIN\n";
            if(rows.empty()) {
                cout << "Рядків не знайдено.\n";
                return;
            }

            for(auto element : rows[0]) cout << element.first << " | ";
            cout << "\n--------------------------------------------------\n";

            for(auto row : rows) {
                for(auto element : row) cout << element.second << " | ";
                cout << endl;
            }
        };
};

class Database {
    private:
        vector<Table*> tables;

        Database() {}
        Database(const Database&) = delete;
        Database& operator=(const Database&) = delete;

    public:
        static Database& getInstance() {
            static Database database;
            return database;
        };

        ~Database() {
            for(Table* table : tables) delete table;
        };

        Table* getTable(const string& name) const {
            for(Table* table : tables) {
                if(table->getName() == name) return table;
            }
            return nullptr;
        };

        Table* createTable(const string& name, vector<Column*>& columns) {
            if(getTable(name) != nullptr) {
                throw invalid_argument("Таблиця вже існує");
            }

            for(Column* column : columns) {
                if(column->isForeignKey()) {
                    Table* foreignTable = getTable(column->getForeignTable());

                    if(foreignTable == nullptr ||
                       foreignTable->getColumn(column->getForeignColumn()) == nullptr) {
                        throw invalid_argument("Неправильний foreign key");
                    }
                }
            }

            Table* table = new Table(name, columns);
            tables.push_back(table);
            return table;
        };

        Row* insert(const string& tableName,
                    const map<string, string>& data) {
            Table* table = getTable(tableName);
            if(table == nullptr) throw invalid_argument("Таблицю не знайдено");

            for(Column* column : table->getColumns()) {
                if(column->isForeignKey()) {
                    auto element = data.find(column->getName());
                    string value = element == data.end() ? "" : element->second;
                    Table* foreignTable = getTable(column->getForeignTable());

                    if(!foreignTable->hasValue(
                        column->getForeignColumn(), value)) {
                        throw invalid_argument("Значення foreign key не існує");
                    }
                }
            }

            return table->insert(data);
        };
};

class TableBuilder {
    private:
        string name;
        vector<Column*> columns;

    public:
        TableBuilder(const string& tableName) : name(tableName) {}

        ~TableBuilder() {
            for(Column* column : columns) delete column;
        };

        TableBuilder& addColumn(const string& columnName, DataType* type,
                                bool nullable = true,
                                bool primaryKey = false,
                                bool foreignKey = false,
                                const string& foreignTable = "",
                                const string& foreignColumn = "") {
            columns.push_back(new Column(
                columnName, type, nullable, primaryKey,
                foreignKey, foreignTable, foreignColumn
            ));
            return *this;
        };

        Table* build(Database& database) {
            Table* table = database.createTable(name, columns);
            columns.clear();
            return table;
        };
};

void runProgram() {
    Database& database = Database::getInstance();

    TableBuilder usersBuilder("users");
    usersBuilder
        .addColumn("id", new IntegerType(), false, true)
        .addColumn("name", new StringType(50), false)
        .addColumn("age", new IntegerType(), false)
        .addColumn("active", new BooleanType(), false)
        .addColumn("birth_date", new DateType(), false);

    Table* users = usersBuilder.build(database);

    TableBuilder ordersBuilder("orders");
    ordersBuilder
        .addColumn("id", new IntegerType(), false, true)
        .addColumn("user_id", new IntegerType(), false, false,
                   true, "users", "id")
        .addColumn("product", new StringType(100), false)
        .addColumn("amount", new IntegerType(), false);

    Table* orders = ordersBuilder.build(database);

    database.insert("users", {
        {"id", "1"}, {"name", "Anna"}, {"age", "20"},
        {"active", "true"}, {"birth_date", "2005-04-15"}
    });

    database.insert("users", {
        {"id", "2"}, {"name", "Bohdan"}, {"age", "25"},
        {"active", "true"}, {"birth_date", "2000-08-10"}
    });

    database.insert("users", {
        {"id", "3"}, {"name", "Olena"}, {"age", "30"},
        {"active", "false"}, {"birth_date", "1995-12-03"}
    });

    // CREATE: додавання замовлень
    database.insert("orders", {
        {"id", "101"}, {"user_id", "1"},
        {"product", "Keyboard"}, {"amount", "1500"}
    });

    database.insert("orders", {
        {"id", "102"}, {"user_id", "2"},
        {"product", "Mouse"}, {"amount", "700"}
    });

    database.insert("orders", {
        {"id", "103"}, {"user_id", "2"},
        {"product", "Monitor"}, {"amount", "5000"}
    });

    users->print();
    orders->print();

    users->update(1, "name", "Anna Maria");

    Row* temporary = users->insert({
        {"id", "4"}, {"name", "Temporary"}, {"age", "18"},
        {"active", "false"}, {"birth_date", "2007-01-01"}
    });
    users->remove(temporary->getID());

    cout << "\nПісля UPDATE та DELETE:\n";
    users->print();

    SimpleQuery query(users);
    query.select({"name", "age"})
         .where("age", ">", "20")
         .sortBy("age", false)
         .print();

    JoinedTable joined(users, orders, "id", "user_id");
    joined.print();

    SimpleQuery orderQuery(orders);
    cout << "\nАгрегатні функції для orders.amount:\n";
    cout << "COUNT = " << orderQuery.count() << endl;
    cout << "SUM = " << orderQuery.sum("amount") << endl;
    cout << fixed << setprecision(2);
    cout << "AVG = " << orderQuery.average("amount") << endl;

    Database& secondLink = Database::getInstance();
    cout << "\nSingleton: "
         << ((&database == &secondLink) ? "працює" : "помилка")
         << endl;

    try {
        database.insert("users", {
            {"id", "1"}, {"name", "Duplicate"}, {"age", "19"},
            {"active", "true"}, {"birth_date", "2006-01-01"}
        });
    }
    catch(const exception& error) {
        cout << "Primary key: " << error.what() << endl;
    }

    try {
        database.insert("orders", {
            {"id", "104"}, {"user_id", "999"},
            {"product", "Laptop"}, {"amount", "30000"}
        });
    }
    catch(const exception& error) {
        cout << "Foreign key: " << error.what() << endl;
    }
};

int main() {
    runProgram();
    return 0;
};
