# VultureDB - Conversational SQL 🦅

VultureDB allows you to interact with your data using **Plain English** (Natural Language). You don't need to know SQL syntax; just tell the engine what you want to do.

## 🗣️ The "Plain English" Workflow

Here is a complete sequence of commands you can type into the **NLQ Interface (Option 14)**.

### 1. Create a Table
Tell the system to build a new structure for you.
> **You type:**
> *"Create a table named employees with fields id name role salary"*
>
> 🟢 **System:**
> *Table 'employees' created with headers [id, name, role, salary].*

---

### 2. Insert Data
Add records naturally.
> **You type:**
> *"Add a new employee 1 Alice Engineer 120000"*
>
> 🟢 **System:**
> *Record inserted.*

> **You type:**
> *"Insert another one 2 Bob Marketer 80000 into employees"*
>
> 🟢 **System:**
> *Record inserted.*

> **You type:**
> *"Add 3 Charlie Intern 30000"*
>
> 🟢 **System:**
> *Record inserted.*

---

### 3. Ask Questions (Select)
Query your data as if asking a colleague.

**Simple Lookup:**
> **You type:**
> *"Show me details for Alice"*
>
> 🟢 **System:**
> ```text
> ID 0: 1 | Alice | Engineer | 120000 |
> ```

**Salary Filter:**
> **You type:**
> *"Find employees earning more than 100000"*
>
> 🟢 **System:**
> ```text
> ID 0: 1 | Alice | Engineer | 120000 |
> ```

**Role Search:**
> **You type:**
> *"Who is the Intern?"*
>
> 🟢 **System:**
> ```text
> ID 2: 3 | Charlie | Intern | 30000 |
> ```

---

### 4. Make Changes (Update)
Modify records using logic.
> **You type:**
> *"Update employees set salary 90000 where name Bob"*
>
> 🟢 **System:**
> *1 records updated.*

---

### 5. Final Check
Verify your changes.
> **You type:**
> *"Show me all employees"*
>
> 🟢 **System:**
> ```text
> ID 0: 1 | Alice | Engineer | 120000 |
> ID 1: 2 | Bob | Marketer | 90000 |
> ID 2: 3 | Charlie | Intern | 30000 |
> ```
