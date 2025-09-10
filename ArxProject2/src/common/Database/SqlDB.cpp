#include "StdAfx.h"
#include "SqlDB.h"
#include "../CadLogger.h"
#include <vector>
#include <sstream>
#include <tchar.h>

// 静态成员变量定义
SQLHENV SqlDB::m_hEnv = SQL_NULL_HENV;
SQLHDBC SqlDB::m_hDbc = SQL_NULL_HDBC;

SqlDB::SqlDB()
{
}

SqlDB::~SqlDB()
{
}

// 初始化数据库连接
bool SqlDB::initDatabase()
{
    try {
        acutPrintf(_T("\n=== Starting database initialization ===\n"));
        
        // 分配环境句柄
        acutPrintf(_T("\nStep 1: Allocating environment handle...\n"));
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_ENV, SQL_NULL_HANDLE, &m_hEnv);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            acutPrintf(_T("\nERROR: Failed to allocate environment handle\n"));
            acutPrintf(_T("Failed to allocate environment handle"));
            return false;
        }
        acutPrintf(_T("\nStep 1: SUCCESS\n"));
        
        // 设置ODBC版本
        acutPrintf(_T("\nStep 2: Setting ODBC version...\n"));
        ret = SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            acutPrintf(_T("\nERROR: Failed to set ODBC version\n"));
            acutPrintf(_T("Failed to set ODBC version"));
            SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
            m_hEnv = SQL_NULL_HENV;
            return false;
        }
        acutPrintf(_T("\nStep 2: SUCCESS\n"));
        
        // 分配连接句柄
        acutPrintf(_T("\nStep 3: Allocating connection handle...\n"));
        ret = SQLAllocHandle(SQL_HANDLE_DBC, m_hEnv, &m_hDbc);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            acutPrintf(_T("\nERROR: Failed to allocate connection handle\n"));
            acutPrintf(_T("Failed to allocate connection handle"));
            SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
            m_hEnv = SQL_NULL_HENV;
            return false;
        }
        acutPrintf(_T("\nStep 3: SUCCESS\n"));
        
        // 设置连接超时
        SQLSetConnectAttr(m_hDbc, SQL_LOGIN_TIMEOUT, (SQLPOINTER)5, 0);
        
        // 连接到数据库
        // 这里使用PostgreSQL的ODBC连接字符串
        acutPrintf(_T("\nStep 4: Connecting to database...\n"));
        std::wstring connStr = L"DRIVER={PostgreSQL Unicode};SERVER=localhost;PORT=5432;DATABASE=postgres;UID=postgres;PWD=1234;";
        
        ret = SQLDriverConnect(m_hDbc, NULL, (SQLWCHAR*)connStr.c_str(), SQL_NTS, NULL, 0, NULL, SQL_DRIVER_NOPROMPT);
        
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            std::wstring error = getLastError();
            acutPrintf(_T("\nERROR: Database connection failed\n"));
            acutPrintf(_T("Error details: %s\n"), error.c_str());
            // 临时注释掉 CadLogger，直接使用 acutPrintf
            // acutPrintf(_T("Database connection failed: %s"), error.c_str());
            
            SQLFreeHandle(SQL_HANDLE_DBC, m_hDbc);
            SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
            m_hDbc = SQL_NULL_HDBC;
            m_hEnv = SQL_NULL_HENV;
            return false;
        }
        acutPrintf(_T("\nStep 4: SUCCESS\n"));
        
        // 直接使用 acutPrintf 而不是 CadLogger
        acutPrintf(_T("\nPostgreSQL database connected successfully!\n"));
        //acutPrintf(_T("PostgreSQL database connected successfully!"));
        return true;
        
    } catch (...) {
        acutPrintf(_T("数据库初始化时发生异常!"));
        return false;
    }
}

// 测试数据库连接
bool SqlDB::testDatabaseConnection()
{
    try {
        if (m_hDbc == SQL_NULL_HDBC) {
           
            return initDatabase();
        }
        
        // 执行简单的测试查询
        SQLHSTMT hStmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &hStmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            acutPrintf(_T("分配语句句柄失败"));
            return false;
        }
        
        ret = SQLExecDirect(hStmt, (SQLWCHAR*)L"SELECT version()", SQL_NTS);
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
            SQLWCHAR version[256];
            SQLLEN indicator;
            
            ret = SQLFetch(hStmt);
            if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
                ret = SQLGetData(hStmt, 1, SQL_C_WCHAR, version, sizeof(version), &indicator);
                if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
                    // 直接使用 acutPrintf 而不是 CadLogger
                    acutPrintf(_T("\nDatabase connection test successful! PostgreSQL version: %s\n"), version);
                    //acutPrintf(_T("数据库连接测试成功! PostgreSQL版本: %s"), version);
                }
            }
            
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return true;
        } else {
            std::wstring error = getLastError();
            acutPrintf(_T("数据库查询测试失败: %s"), error.c_str());
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
        
        return false;
        
    } catch (...) {
        acutPrintf(_T("数据库连接测试时发生异常!"));
        return false;
    }
}

// 关闭数据库连接
void SqlDB::closeDatabase()
{
    if (m_hDbc != SQL_NULL_HDBC) {
        SQLDisconnect(m_hDbc);
        SQLFreeHandle(SQL_HANDLE_DBC, m_hDbc);
        m_hDbc = SQL_NULL_HDBC;
       acutPrintf(_T("数据库连接已关闭!"));
    }
    
    if (m_hEnv != SQL_NULL_HENV) {
        SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
        m_hEnv = SQL_NULL_HENV;
    }
}

// 检查数据库是否打开
bool SqlDB::isDatabaseOpen()
{
    return (m_hDbc != SQL_NULL_HDBC);
}

// 执行通用查询（带错误信息返回）
bool SqlDB::executeQuery(const std::wstring& sql, std::wstring& errorMsg)
{
    try {
        acutPrintf(_T("准备执行SQL: %s"), sql.c_str());
        
        if (m_hDbc == SQL_NULL_HDBC) {
            errorMsg = L"数据库未连接";
            acutPrintf(_T("错误: 数据库未连接"));
            return false;
        }
        
        SQLHSTMT hStmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &hStmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            errorMsg = L"分配语句句柄失败";
            acutPrintf(_T("错误: 分配语句句柄失败"));
            return false;
        }
        
        acutPrintf(_T("开始执行SQL语句"));
        ret = SQLExecDirect(hStmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
        acutPrintf(_T("SQL执行返回码: %d"), ret);
        
        // 修改判断条件：SQL_NO_DATA (100) 对于DELETE/UPDATE/INSERT是正常的
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO || ret == SQL_NO_DATA) {
            acutPrintf(_T("SQL执行成功"));
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return true;
        } else {
            acutPrintf(_T("SQL执行失败，返回码: %d"), ret);
            // 获取语句级别的错误信息
            errorMsg = getStatementError(hStmt);
            acutPrintf(_T("错误信息: %s"), errorMsg.c_str());
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return false;
        }
        
    } catch (...) {
        errorMsg = L"执行查询时发生异常";
        acutPrintf(_T("执行查询时发生异常"));
        return false;
    }
}

// 新增方法：获取语句级别的错误信息
std::wstring SqlDB::getStatementError(SQLHSTMT hStmt)
{
    SQLWCHAR sqlState[6], messageText[512];
    SQLINTEGER nativeError;
    SQLSMALLINT textLength;
    
    if (hStmt != SQL_NULL_HSTMT) {
        SQLRETURN ret = SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, 
                                     messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
            return std::wstring(messageText);
        }
    }
    
    // 如果获取语句错误失败，尝试获取连接错误
    return getLastError();
}

// 执行通用查询（简单版本）
bool SqlDB::executeQuery(const std::wstring& sql)
{
    std::wstring errorMsg;
    return executeQuery(sql, errorMsg);
}

// 添加查询结果返回方法
bool SqlDB::executeSelectQuery(const std::wstring& sql, std::vector<std::vector<std::wstring>>& results, std::wstring& errorMsg)
{
    results.clear();
    
    try {
        if (m_hDbc == SQL_NULL_HDBC) {
            errorMsg = L"数据库未连接";
            return false;
        }
        
        SQLHSTMT hStmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &hStmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            errorMsg = L"分配语句句柄失败";
            return false;
        }
        
        ret = SQLExecDirect(hStmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
            // 获取列数
            SQLSMALLINT columnCount;
            SQLNumResultCols(hStmt, &columnCount);
            
            // 读取数据
            while (SQLFetch(hStmt) == SQL_SUCCESS) {
                std::vector<std::wstring> row;
                
                for (int i = 1; i <= columnCount; i++) {
                    SQLWCHAR buffer[1024];
                    SQLLEN indicator;
                    
                    ret = SQLGetData(hStmt, i, SQL_C_WCHAR, buffer, sizeof(buffer), &indicator);
                    if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
                        if (indicator == SQL_NULL_DATA) {
                            row.push_back(L"");
                        } else {
                            row.push_back(std::wstring(buffer));
                        }
                    } else {
                        row.push_back(L"");
                    }
                }
                
                results.push_back(row);
            }
        } else {
            errorMsg = getLastError();
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return false;
        }
        
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return true;
        
    } catch (...) {
        errorMsg = L"执行查询时发生异常";
        return false;
    }
}

// 创建测试表
bool SqlDB::createTestTables()
{
    try {
       acutPrintf(_T("=== 开始创建测试表 ==="));
        
        if (!createUserTable()) {
            acutPrintf(_T("创建用户表失败"));
            return false;
        }
        
        if (!createProjectTable()) {
            acutPrintf(_T("创建项目表失败"));
            return false;
        }
        
        if (!createDrawingTable()) {
            acutPrintf(_T("创建图纸表失败"));
            return false;
        }
        
       acutPrintf(_T("=== 测试表创建完成 ==="));
        return true;
        
    } catch (...) {
        acutPrintf(_T("创建测试表时发生异常"));
        return false;
    }
}

// 插入测试数据
bool SqlDB::insertTestData()
{
    try {
       acutPrintf(_T("=== 开始插入测试数据 ==="));
        
        if (!insertUserData()) {
            acutPrintf(_T("插入用户数据失败"));
            return false;
        }
        
        if (!insertProjectData()) {
            acutPrintf(_T("插入项目数据失败"));
            return false;
        }
        
        if (!insertDrawingData()) {
            acutPrintf(_T("插入图纸数据失败"));
            return false;
        }
        
       acutPrintf(_T("=== 测试数据插入完成 ==="));
        return true;
        
    } catch (...) {
        acutPrintf(_T("插入测试数据时发生异常"));
        return false;
    }
}

// 显示测试数据
bool SqlDB::showTestData()
{
    try {
       acutPrintf(_T("=== 开始显示测试数据 ==="));
        
        // 查询用户表
        std::wstring sql = L"SELECT id, name, email FROM test_users ORDER BY id";
        std::wstring errorMsg;
        
        if (m_hDbc == SQL_NULL_HDBC) {
            acutPrintf(_T("数据库未连接"));
            return false;
        }
        
        SQLHSTMT hStmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &hStmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            acutPrintf(_T("分配语句句柄失败"));
            return false;
        }
        
        ret = SQLExecDirect(hStmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
           acutPrintf(_T("用户表数据:"));
            
            SQLINTEGER id;
            SQLWCHAR name[100], email[100];
            SQLLEN idLen, nameLen, emailLen;
            
            while (SQLFetch(hStmt) == SQL_SUCCESS) {
                SQLGetData(hStmt, 1, SQL_C_LONG, &id, 0, &idLen);
                SQLGetData(hStmt, 2, SQL_C_WCHAR, name, sizeof(name), &nameLen);
                SQLGetData(hStmt, 3, SQL_C_WCHAR, email, sizeof(email), &emailLen);
                
               acutPrintf(_T("  ID: %d, 姓名: %s, 邮箱: %s"), id, name, email);
            }
        }
        
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        
       acutPrintf(_T("=== 测试数据显示完成 ==="));
        return true;
        
    } catch (...) {
        acutPrintf(_T("显示测试数据时发生异常"));
        return false;
    }
}

// 查询测试数据
bool SqlDB::queryTestData()
{
    return showTestData(); // 简单实现，直接调用显示方法
}

// 清空测试数据
bool SqlDB::clearTestData()
{
    try {
        std::wstring errorMsg;
        
        if (!executeQuery(L"DROP TABLE IF EXISTS test_drawings", errorMsg)) {
            acutPrintf(_T("删除图纸表失败: %s"), errorMsg.c_str());
            return false;
        }
        
        if (!executeQuery(L"DROP TABLE IF EXISTS test_projects", errorMsg)) {
            acutPrintf(_T("删除项目表失败: %s"), errorMsg.c_str());
            return false;
        }
        
        if (!executeQuery(L"DROP TABLE IF EXISTS test_users", errorMsg)) {
            acutPrintf(_T("删除用户表失败: %s"), errorMsg.c_str());
            return false;
        }
        
       acutPrintf(_T("测试数据清空完成"));
        return true;
        
    } catch (...) {
        acutPrintf(_T("清空测试数据时发生异常"));
        return false;
    }
}

// 保存图纸数据
bool SqlDB::saveSheetData(const std::vector<DbSheetData>& sheetList, std::wstring& errorMsg)
{
    try {
        // 先清空现有数据
        if (!executeQuery(L"DELETE FROM sheet_list_data", errorMsg)) {
            return false;
        }
        
        // 插入新数据
        for (const auto& sheet : sheetList) {
            std::wstringstream sql;
            sql << L"INSERT INTO sheet_list_data (name, building_name, specialty, format, status, version, design_unit, create_time, creator, is_selected, file_path) VALUES (";
            sql << L"'" << sheet.name << L"', ";
            sql << L"'" << sheet.buildingName << L"', ";
            sql << L"'" << sheet.specialty << L"', ";
            sql << L"'" << sheet.format << L"', ";
            sql << L"'" << sheet.status << L"', ";
            sql << L"'" << sheet.version << L"', ";
            sql << L"'" << sheet.designUnit << L"', ";
            sql << L"'" << sheet.createTime << L"', ";
            sql << L"'" << sheet.creator << L"', ";
            sql << (sheet.isSelected ? L"true" : L"false") << L", ";
            sql << L"'" << sheet.filePath << L"')";
            
            if (!executeQuery(sql.str(), errorMsg)) {
                return false;
            }
        }
        
        return true;
        
    } catch (...) {
        errorMsg = L"保存图纸数据时发生异常";
        return false;
    }
}

// 加载图纸数据
bool SqlDB::loadSheetData(std::vector<DbSheetData>& sheetList, std::wstring& errorMsg)
{
    sheetList.clear();
    
    try {
        std::wstring sql = L"SELECT id, name, building_name, specialty, format, status, version, design_unit, create_time, creator, is_selected, file_path FROM sheet_list_data ORDER BY id";
        
        if (m_hDbc == SQL_NULL_HDBC) {
            errorMsg = L"数据库未连接";
            return false;
        }
        
        SQLHSTMT hStmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &hStmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            errorMsg = L"分配语句句柄失败";
            return false;
        }
        
        ret = SQLExecDirect(hStmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
            SQLINTEGER id;
            SQLWCHAR name[256], buildingName[256], specialty[256], format[256];
            SQLWCHAR status[256], version[256], designUnit[256], createTime[256];
            SQLWCHAR creator[256], filePath[512];
            SQLINTEGER isSelected;
            SQLLEN len;
            
            while (SQLFetch(hStmt) == SQL_SUCCESS) {
                DbSheetData sheet;
                
                SQLGetData(hStmt, 1, SQL_C_LONG, &id, 0, &len);
                SQLGetData(hStmt, 2, SQL_C_WCHAR, name, sizeof(name), &len);
                SQLGetData(hStmt, 3, SQL_C_WCHAR, buildingName, sizeof(buildingName), &len);
                SQLGetData(hStmt, 4, SQL_C_WCHAR, specialty, sizeof(specialty), &len);
                SQLGetData(hStmt, 5, SQL_C_WCHAR, format, sizeof(format), &len);
                SQLGetData(hStmt, 6, SQL_C_WCHAR, status, sizeof(status), &len);
                SQLGetData(hStmt, 7, SQL_C_WCHAR, version, sizeof(version), &len);
                SQLGetData(hStmt, 8, SQL_C_WCHAR, designUnit, sizeof(designUnit), &len);
                SQLGetData(hStmt, 9, SQL_C_WCHAR, createTime, sizeof(createTime), &len);
                SQLGetData(hStmt, 10, SQL_C_WCHAR, creator, sizeof(creator), &len);
                SQLGetData(hStmt, 11, SQL_C_LONG, &isSelected, 0, &len);
                SQLGetData(hStmt, 12, SQL_C_WCHAR, filePath, sizeof(filePath), &len);
                
                sheet.id = id;
                sheet.name = name;
                sheet.buildingName = buildingName;
                sheet.specialty = specialty;
                sheet.format = format;
                sheet.status = status;
                sheet.version = version;
                sheet.designUnit = designUnit;
                sheet.createTime = createTime;
                sheet.creator = creator;
                sheet.isSelected = (isSelected != 0);
                sheet.filePath = filePath;
                
                sheetList.push_back(sheet);
            }
        }
        
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        return true;
        
    } catch (...) {
        errorMsg = L"加载图纸数据时发生异常";
        return false;
    }
}

// 清空图纸数据
bool SqlDB::clearSheetData(std::wstring& errorMsg)
{
    return executeQuery(L"DELETE FROM sheet_data", errorMsg);
}





// 创建指定名称和结构的表
bool SqlDB::createSheetTable(const std::wstring& tableName, const std::wstring& tableStructure)
{
    std::wstring sql = L"CREATE TABLE IF NOT EXISTS " + tableName + L" " + tableStructure;
    std::wstring errorMsg;
    return executeQuery(sql, errorMsg);
}

// 字段管理方法
bool SqlDB::addFieldToTable(const std::wstring& tableName, const std::wstring& fieldName, const std::wstring& fieldType, const std::wstring& defaultValue)
{
    std::wstring sql = L"ALTER TABLE " + tableName + L" ADD COLUMN " + fieldName + L" " + fieldType;
    if (!defaultValue.empty()) {
        sql += L" DEFAULT " + defaultValue;
    }
    
    std::wstring errorMsg;
    return executeQuery(sql, errorMsg);
}

bool SqlDB::removeFieldFromTable(const std::wstring& tableName, const std::wstring& fieldName)
{
    std::wstring sql = L"ALTER TABLE " + tableName + L" DROP COLUMN " + fieldName;
    std::wstring errorMsg;
    return executeQuery(sql, errorMsg);
}

bool SqlDB::fieldExists(const std::wstring& tableName, const std::wstring& fieldName)
{
    // 这里需要查询系统表来检查字段是否存在
    // 具体实现依赖于数据库类型
    return true; // 暂时返回true
}

std::vector<std::wstring> SqlDB::getTableFields(const std::wstring& tableName)
{
    std::vector<std::wstring> fields;
    // 这里需要查询系统表来获取字段列表
    // 具体实现依赖于数据库类型
    return fields;
}

// 新增：执行非查询语句（INSERT、UPDATE、DELETE）
bool SqlDB::executeNonQuery(const std::wstring& sql, std::wstring& errorMsg)
{
    try {
        acutPrintf(_T("准备执行非查询SQL: %s"), sql.c_str());
        
        if (m_hDbc == SQL_NULL_HDBC) {
            errorMsg = L"数据库未连接";
            acutPrintf(_T("错误: 数据库未连接"));
            return false;
        }
        
        SQLHSTMT hStmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &hStmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            errorMsg = L"分配语句句柄失败";
            acutPrintf(_T("错误: 分配语句句柄失败"));
            return false;
        }
        
        acutPrintf(_T("开始执行SQL语句"));
        ret = SQLExecDirect(hStmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
        acutPrintf(_T("SQL执行返回码: %d"), ret);
        
        // 对于非查询语句，SQL_NO_DATA是正常的返回值
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO || ret == SQL_NO_DATA) {
            // 获取受影响的行数
            SQLLEN rowCount = 0;
            SQLRowCount(hStmt, &rowCount);
            acutPrintf(_T("SQL执行成功，影响行数: %d"), (int)rowCount);
            
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return true;
        } else {
            acutPrintf(_T("SQL执行失败，返回码: %d"), ret);
            errorMsg = getStatementError(hStmt);
            acutPrintf(_T("错误信息: %s"), errorMsg.c_str());
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return false;
        }
        
    } catch (...) {
        errorMsg = L"执行非查询语句时发生异常";
        acutPrintf(_T("执行非查询语句时发生异常"));
        return false;
    }
}

// 私有辅助方法
bool SqlDB::createUserTable()
{
    std::wstring sql = LR"(
        CREATE TABLE IF NOT EXISTS test_users (
            id SERIAL PRIMARY KEY,
            name VARCHAR(100) NOT NULL,
            email VARCHAR(200) UNIQUE NOT NULL,
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    std::wstring errorMsg;
    return executeQuery(sql, errorMsg);
}

bool SqlDB::createProjectTable()
{
    std::wstring sql = LR"(
        CREATE TABLE IF NOT EXISTS test_projects (
            id SERIAL PRIMARY KEY,
            name VARCHAR(200) NOT NULL,
            description TEXT,
            user_id INTEGER REFERENCES test_users(id),
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    std::wstring errorMsg;
    return executeQuery(sql, errorMsg);
}

bool SqlDB::createDrawingTable()
{
    std::wstring sql = LR"(
        CREATE TABLE IF NOT EXISTS test_drawings (
            id SERIAL PRIMARY KEY,
            name VARCHAR(200) NOT NULL,
            file_path VARCHAR(500),
            project_id INTEGER REFERENCES test_projects(id),
            created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
        )
    )";
    
    std::wstring errorMsg;
    return executeQuery(sql, errorMsg);
}

bool SqlDB::insertUserData()
{
    std::vector<std::wstring> users = {
        L"INSERT INTO test_users (name, email) VALUES ('张三', 'zhangsan@example.com')",
        L"INSERT INTO test_users (name, email) VALUES ('李四', 'lisi@example.com')",
        L"INSERT INTO test_users (name, email) VALUES ('王五', 'wangwu@example.com')"
    };
    
    std::wstring errorMsg;
    for (const auto& sql : users) {
        if (!executeQuery(sql, errorMsg)) {
            acutPrintf(_T("插入用户数据失败: %s"), errorMsg.c_str());
            return false;
        }
    }
    
    return true;
}

bool SqlDB::insertProjectData()
{
    std::vector<std::wstring> projects = {
        L"INSERT INTO test_projects (name, description, user_id) VALUES ('项目A', '测试项目A描述', 1)",
        L"INSERT INTO test_projects (name, description, user_id) VALUES ('项目B', '测试项目B描述', 2)",
        L"INSERT INTO test_projects (name, description, user_id) VALUES ('项目C', '测试项目C描述', 1)"
    };
    
    std::wstring errorMsg;
    for (const auto& sql : projects) {
        if (!executeQuery(sql, errorMsg)) {
            acutPrintf(_T("插入项目数据失败: %s"), errorMsg.c_str());
            return false;
        }
    }
    
    return true;
}

bool SqlDB::insertDrawingData()
{
    std::vector<std::wstring> drawings = {
        L"INSERT INTO test_drawings (name, file_path, project_id) VALUES ('图纸1', 'D:/drawings/drawing1.dwg', 1)",
        L"INSERT INTO test_drawings (name, file_path, project_id) VALUES ('图纸2', 'D:/drawings/drawing2.dwg', 1)",
        L"INSERT INTO test_drawings (name, file_path, project_id) VALUES ('图纸3', 'D:/drawings/drawing3.dwg', 2)"
    };
    
    std::wstring errorMsg;
    for (const auto& sql : drawings) {
        if (!executeQuery(sql, errorMsg)) {
            acutPrintf(_T("插入图纸数据失败: %s"), errorMsg.c_str());
            return false;
        }
    }
    
    return true;
}

// 改进现有的getLastError方法
std::wstring SqlDB::getLastError()
{
    SQLWCHAR sqlState[6], messageText[512];
    SQLINTEGER nativeError;
    SQLSMALLINT textLength;
    
    if (m_hDbc != SQL_NULL_HDBC) {
        SQLRETURN ret = SQLGetDiagRec(SQL_HANDLE_DBC, m_hDbc, 1, sqlState, &nativeError, 
                                     messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
            return std::wstring(messageText);
        }
    }
    
    return L"未知错误";
} 



