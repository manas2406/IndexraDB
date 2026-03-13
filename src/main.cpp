#include <iostream>
#include <sstream>
#include "../include/database.hpp"
#include "../include/join.hpp"
#include "../include/fileio.hpp"
#include "../include/ml.hpp"
#include "../include/nlq_ml/pipeline.hpp"

#include "../include/llm_bridge.hpp"

namespace NLQ {
    void executeSmart(::Query& q, Database& db);
    void executeQuery(const NLQ::Query& q, Database& db);
    void executeQueries(const std::vector<NLQ::Query>& queries, Database& db);
}

void printMenu() {
    cout << "\n=== Antigravity DB (B+ Tree) ===\n";
    cout << "1. Create Table\n";
    cout << "2. Insert Record\n";
    cout << "3. Edit Record\n";
    cout << "4. Delete Record\n";
    cout << "5. View Table\n";
    cout << "6. Search (Index)\n";
    cout << "7. Range Search (Index)\n";
    cout << "8. Join Tables\n";
    cout << "9. List Tables\n";
    cout << "10. Save Database\n";
    cout << "11. Load Database\n";
    cout << "12. Predict (Linear Regression)\n";
    cout << "13. Cluster (K-Means)\n";
    cout << "14. Natural Language Query (ML Beta)\n";
    cout << "15. LLM NLQ (Gemini + LangChain)\n";
    cout << "0. Exit\n";
    cout << "Option: ";
}

int main() {
    Database db;
    
    while (true) {
        printMenu();
        int choice;
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(1000, '\n');
            continue;
        }
        
        if (choice == 0) break;
        
        // Actions
        if (choice == 1) { // Create
            string name; cout << "Table Name: "; cin >> name;
            cout << "Enter Field Names (space separated, end with .): ";
            vector<string> fields; string f;
            while (cin >> f && f != ".") fields.push_back(f);
            db.createTable(name, fields);
            cout << "Table created.\n";
        }
        else if (choice == 2) { // Insert
            string name; cout << "Table Name: "; cin >> name;
            Table* t = db.getTable(name);
            if (!t) { cout << "Table not found.\n"; continue; }
            map<string, string> row; cout << "Enter values for:\n";
            for (const auto& field : t->fields) {
                cout << field << ": "; string val; cin >> val; row[field] = val;
            }
            t->insert(row); cout << "Record inserted.\n";
        }
        else if (choice == 3) { 
             string name; cout << "Table Name: "; cin >> name; 
             Table* t = db.getTable(name); if(t){ int id; string f,v; cout<<"ID: "; cin>>id; cout<<"Field: "; cin>>f; cout<<"Val: "; cin>>v; t->updateRow(id,f,v); }
        }
        else if (choice == 4) {
             string name; cout << "Table Name: "; cin >> name;
             Table* t = db.getTable(name); if(t){ int id; cout<<"ID: "; cin>>id; t->deleteRow(id); }
        }
        else if (choice == 5) {
             string name; cout << "Table Name: "; cin >> name;
             Table* t = db.getTable(name); if(t) cout<<t->view();
        }
        else if (choice == 6) {
             string name,f,v; cout<<"Table: "; cin>>name; Table* t=db.getTable(name);
             if(t){ cout<<"Field: "; cin>>f; cout<<"Val: "; cin>>v; for(int id:t->search(f,v)) cout<<"ID "<<id<<"\n"; }
        }
        else if (choice == 7) {
             string name,f,s,e; cout<<"Table: "; cin>>name; Table* t=db.getTable(name);
             if(t){ cout<<"Field: "; cin>>f; cout<<"Start: "; cin>>s; cout<<"End: "; cin>>e; for(int id:t->rangeSearch(f,s,e)) cout<<"ID "<<id<<"\n"; }
        }
        else if (choice == 8) {
             string t1, t2, f; cout<<"T1: "; cin>>t1; cout<<"T2: "; cin>>t2; cout<<"Join Field: "; cin>>f;
             auto res = joinTables(db,t1,t2,f);
             for(auto r: res) { for(auto p:r) cout<<p.first<<":"<<p.second<<" "; cout<<"\n"; }
        }
        else if (choice == 9) { for(auto t: db.listTables()) cout<<t<<"\n"; }
        else if (choice == 10) saveDatabase(db, "data");
        else if (choice == 11) loadDatabase(db, "data");

        else if (choice == 12) { // Predict
            string name, target, feature; double val;
            cout << "Table: "; cin >> name; Table* t = db.getTable(name);
            if(t) { cout << "Target: "; cin >> target; cout << "Feat: "; cin >> feature; cout << "Val: "; cin >> val; cout << ML::predictLinear(t,target,feature,val) << "\n"; }
        }
        else if (choice == 13) { // Cluster
            string name; int k; cout << "Table: "; cin >> name;
            Table* t = db.getTable(name);
            if(t) { 
                cout << "K: "; cin >> k; vector<string> c; string s; cout << "Cols (. end): "; 
                while(cin>>s && s!=".") c.push_back(s); 
                auto res = ML::kMeans(t,k,c);
                for(auto [id,m]: res) { cout << "Cluster " << id << ": " << m.size() << "\n"; }
            }
        }
        else if (choice == 14) { // NLQ (ML Pipeline)
            string queryStr;
            cout << "Enter Query (ML): ";
            cin.ignore(); 
            getline(cin, queryStr);
            Query q = NLQ_ML::processParams(queryStr);
            NLQ::executeSmart(q, db);
        }
        else if (choice == 15) { // LLM NLQ
            string queryStr;
            cout << "Enter Query (English): ";
            cin.ignore();
            getline(cin, queryStr);
            
            // 1. Export Schema
            string schema = exportSchemaJSON(db);
            
            // 2. Prepare Payload
            // Using a simple manual JSON builder to avoid complexity
            string payload = "{\"query\": \"" + escapeJSON(queryStr) + "\", \"schema\": " + schema + "}";
            
            cout << "Thinking (sending to Gemini)...\n";
            try {
                // 3. Call LLM
                string response = httpPost("http://localhost:8000/nlq", payload);
                if (response.empty() || response.find("error") != string::npos) {
                    cout << "LLM Error: " << response << "\n";
                    continue;
                }
                
                cout << "Plan: " << response << "\n";
                
                // 4. Parse Plan
                vector<NLQ::Query> queries = parseJSONToQueries(response);
                
                // 5. Execute
                NLQ::executeQueries(queries, db);
                
            } catch (const exception& e) {
                cout << "Error: " << e.what() << "\n";
            }
        }
    }
    return 0;
}
