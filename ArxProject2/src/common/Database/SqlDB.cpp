#include "StdAfx.h"
#include "SqlDB.h"
#include "../CadLogger.h"
#include "../../services/SearchTextInDwg.h"  // 添加这一行来包含 TextSearchResult 的定义
#include <vector>
#include <sstream>
#include <tchar.h>
#include <algorithm>

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
            CadLogger::LogError(_T("Failed to allocate environment handle"));
            return false;
        }
        acutPrintf(_T("\nStep 1: SUCCESS\n"));
        
        // 设置ODBC版本
        acutPrintf(_T("\nStep 2: Setting ODBC version...\n"));
        ret = SQLSetEnvAttr(m_hEnv, SQL_ATTR_ODBC_VERSION, (SQLPOINTER)SQL_OV_ODBC3, 0);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            acutPrintf(_T("\nERROR: Failed to set ODBC version\n"));
            CadLogger::LogError(_T("Failed to set ODBC version"));
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
            CadLogger::LogError(_T("Failed to allocate connection handle"));
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
            // CadLogger::LogError(_T("Database connection failed: %s"), error.c_str());
            
            SQLFreeHandle(SQL_HANDLE_DBC, m_hDbc);
            SQLFreeHandle(SQL_HANDLE_ENV, m_hEnv);
            m_hDbc = SQL_NULL_HDBC;
            m_hEnv = SQL_NULL_HENV;
            return false;
        }
        acutPrintf(_T("\nStep 4: SUCCESS\n"));
        
        // 直接使用 acutPrintf 而不是 CadLogger
        acutPrintf(_T("\nPostgreSQL database connected successfully!\n"));
        // CadLogger::LogInfo(_T("PostgreSQL database connected successfully!"));
        return true;
        
    } catch (...) {
        CadLogger::LogError(_T("数据库初始化时发生异常!"));
        return false;
    }
}

// 测试数据库连接
bool SqlDB::testDatabaseConnection()
{
    try {
        if (m_hDbc == SQL_NULL_HDBC) {
            CadLogger::LogWarning(_T("数据库未连接，尝试重新连接..."));
            return initDatabase();
        }
        
        // 执行简单的测试查询
        SQLHSTMT hStmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &hStmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            CadLogger::LogError(_T("分配语句句柄失败"));
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
                    // CadLogger::LogInfo(_T("数据库连接测试成功! PostgreSQL版本: %s"), version);
                }
            }
            
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return true;
        } else {
            std::wstring error = getLastError();
            CadLogger::LogError(_T("数据库查询测试失败: %s"), error.c_str());
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        }
        
        return false;
        
    } catch (...) {
        CadLogger::LogError(_T("数据库连接测试时发生异常!"));
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
        CadLogger::LogInfo(_T("数据库连接已关闭!"));
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
        if (m_hDbc == SQL_NULL_HDBC) {
            errorMsg = L"数据库未连接";
            return false;
        }
        
        SQLHSTMT hStmt = SQL_NULL_HSTMT;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &hStmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            errorMsg = L"分配语句句柄失败，返回码: " + std::to_wstring(ret);
            return false;
        }
        
        ret = SQLExecDirect(hStmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
        
        // 添加详细的返回码日志
        acutPrintf(_T("SQLExecDirect返回码: %d\n"), ret);
        
        // 修改成功条件：包含 SQL_NO_DATA (100)
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO || ret == SQL_NO_DATA) {
            if (ret == SQL_NO_DATA) {
                acutPrintf(_T("SQL执行成功，但没有数据返回（这对DELETE操作是正常的）\n"));
            } else {
                acutPrintf(_T("SQL执行成功\n"));
            }
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return true;
        } else {
            // 获取详细错误信息
            SQLWCHAR sqlState[6] = {0};
            SQLWCHAR messageText[1024] = {0};
            SQLINTEGER nativeError = 0;
            SQLSMALLINT textLength = 0;
            
            SQLRETURN diagRet = SQLGetDiagRec(SQL_HANDLE_STMT, hStmt, 1, sqlState, &nativeError, 
                                            messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);
            
            if (diagRet == SQL_SUCCESS || diagRet == SQL_SUCCESS_WITH_INFO) {
                if (textLength > 0 && textLength < (sizeof(messageText)/sizeof(SQLWCHAR))) {
                    messageText[textLength] = L'\0';
                }
                
                errorMsg = L"SQL执行失败 - 返回码: " + std::to_wstring(ret) +
                          L", SQL状态: " + std::wstring(sqlState) + 
                          L", 错误代码: " + std::to_wstring(nativeError) + 
                          L", 消息: " + std::wstring(messageText);
            } else {
                errorMsg = L"SQL执行失败，返回码: " + std::to_wstring(ret) + L", 无法获取详细错误信息";
            }
            
            acutPrintf(_T("SQL执行失败: %s\n"), errorMsg.c_str());
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return false;
        }
        
    } catch (...) {
        errorMsg = L"执行查询时发生异常";
        return false;
    }
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
        CadLogger::LogInfo(_T("=== 开始创建测试表 ==="));
        
        if (!createUserTable()) {
            CadLogger::LogError(_T("创建用户表失败"));
            return false;
        }
        
        if (!createProjectTable()) {
            CadLogger::LogError(_T("创建项目表失败"));
            return false;
        }
        
        if (!createDrawingTable()) {
            CadLogger::LogError(_T("创建图纸表失败"));
            return false;
        }
        
        CadLogger::LogInfo(_T("=== 测试表创建完成 ==="));
        return true;
        
    } catch (...) {
        CadLogger::LogError(_T("创建测试表时发生异常"));
        return false;
    }
}

// 插入测试数据
bool SqlDB::insertTestData()
{
    try {
        CadLogger::LogInfo(_T("=== 开始插入测试数据 ==="));
        
        if (!insertUserData()) {
            CadLogger::LogError(_T("插入用户数据失败"));
            return false;
        }
        
        if (!insertProjectData()) {
            CadLogger::LogError(_T("插入项目数据失败"));
            return false;
        }
        
        if (!insertDrawingData()) {
            CadLogger::LogError(_T("插入图纸数据失败"));
            return false;
        }
        
        CadLogger::LogInfo(_T("=== 测试数据插入完成 ==="));
        return true;
        
    } catch (...) {
        CadLogger::LogError(_T("插入测试数据时发生异常"));
        return false;
    }
}

// 显示测试数据
bool SqlDB::showTestData()
{
    try {
        CadLogger::LogInfo(_T("=== 开始显示测试数据 ==="));
        
        // 查询用户表
        std::wstring sql = L"SELECT id, name, email FROM test_users ORDER BY id";
        std::wstring errorMsg;
        
        if (m_hDbc == SQL_NULL_HDBC) {
            CadLogger::LogError(_T("数据库未连接"));
            return false;
        }
        
        SQLHSTMT hStmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &hStmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            CadLogger::LogError(_T("分配语句句柄失败"));
            return false;
        }
        
        ret = SQLExecDirect(hStmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
            CadLogger::LogInfo(_T("用户表数据:"));
            
            SQLINTEGER id;
            SQLWCHAR name[100], email[100];
            SQLLEN idLen, nameLen, emailLen;
            
            while (SQLFetch(hStmt) == SQL_SUCCESS) {
                SQLGetData(hStmt, 1, SQL_C_LONG, &id, 0, &idLen);
                SQLGetData(hStmt, 2, SQL_C_WCHAR, name, sizeof(name), &nameLen);
                SQLGetData(hStmt, 3, SQL_C_WCHAR, email, sizeof(email), &emailLen);
                
                CadLogger::LogInfo(_T("  ID: %d, 姓名: %s, 邮箱: %s"), id, name, email);
            }
        }
        
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
        
        CadLogger::LogInfo(_T("=== 测试数据显示完成 ==="));
        return true;
        
    } catch (...) {
        CadLogger::LogError(_T("显示测试数据时发生异常"));
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
            CadLogger::LogError(_T("删除图纸表失败: %s"), errorMsg.c_str());
            return false;
        }
        
        if (!executeQuery(L"DROP TABLE IF EXISTS test_projects", errorMsg)) {
            CadLogger::LogError(_T("删除项目表失败: %s"), errorMsg.c_str());
            return false;
        }
        
        if (!executeQuery(L"DROP TABLE IF EXISTS test_users", errorMsg)) {
            CadLogger::LogError(_T("删除用户表失败: %s"), errorMsg.c_str());
            return false;
        }
        
        CadLogger::LogInfo(_T("测试数据清空完成"));
        return true;
        
    } catch (...) {
        CadLogger::LogError(_T("清空测试数据时发生异常"));
        return false;
    }
}

// 保存图纸数据
bool SqlDB::saveSheetData(const std::vector<DbSheetData>& sheetList, std::wstring& errorMsg)
{
    try {
        // 先清空现有数据
        if (!executeQuery(L"DELETE FROM sheet_data", errorMsg)) {
            return false;
        }
        
        // 使用预处理语句插入新数据
        SQLHSTMT hStmt;
        SQLRETURN ret = SQLAllocHandle(SQL_HANDLE_STMT, m_hDbc, &hStmt);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            errorMsg = L"分配语句句柄失败";
            return false;
        }
        
        // 准备预处理语句
        std::wstring sql = L"INSERT INTO sheet_data (name, building_name, specialty, format, status, version, design_unit, create_time, creator, is_selected, file_path) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)";
        
        ret = SQLPrepare(hStmt, (SQLWCHAR*)sql.c_str(), SQL_NTS);
        if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
            errorMsg = L"准备SQL语句失败";
            SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
            return false;
        }
        
        // 插入每条数据
        for (const auto& sheet : sheetList) {
            // 绑定参数
            SQLBindParameter(hStmt, 1, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, (SQLPOINTER)sheet.name.c_str(), 0, NULL);
            SQLBindParameter(hStmt, 2, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, (SQLPOINTER)sheet.buildingName.c_str(), 0, NULL);
            SQLBindParameter(hStmt, 3, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 100, 0, (SQLPOINTER)sheet.specialty.c_str(), 0, NULL);
            SQLBindParameter(hStmt, 4, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 50, 0, (SQLPOINTER)sheet.format.c_str(), 0, NULL);
            SQLBindParameter(hStmt, 5, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 50, 0, (SQLPOINTER)sheet.status.c_str(), 0, NULL);
            SQLBindParameter(hStmt, 6, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 20, 0, (SQLPOINTER)sheet.version.c_str(), 0, NULL);
            SQLBindParameter(hStmt, 7, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 255, 0, (SQLPOINTER)sheet.designUnit.c_str(), 0, NULL);
            SQLBindParameter(hStmt, 8, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 50, 0, (SQLPOINTER)sheet.createTime.c_str(), 0, NULL);
            SQLBindParameter(hStmt, 9, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 100, 0, (SQLPOINTER)sheet.creator.c_str(), 0, NULL);
            
            SQLINTEGER isSelectedValue = sheet.isSelected ? 1 : 0;
            SQLBindParameter(hStmt, 10, SQL_PARAM_INPUT, SQL_C_LONG, SQL_INTEGER, 0, 0, &isSelectedValue, 0, NULL);
            SQLBindParameter(hStmt, 11, SQL_PARAM_INPUT, SQL_C_WCHAR, SQL_WVARCHAR, 512, 0, (SQLPOINTER)sheet.filePath.c_str(), 0, NULL);
            
            // 执行语句
            ret = SQLExecute(hStmt);
            if (ret != SQL_SUCCESS && ret != SQL_SUCCESS_WITH_INFO) {
                errorMsg = L"执行插入语句失败: " + getLastError();
                SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
                return false;
            }
        }
        
        SQLFreeHandle(SQL_HANDLE_STMT, hStmt);
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
        std::wstring sql = L"SELECT id, name, building_name, specialty, format, status, version, design_unit, create_time, creator, is_selected, file_path FROM sheet_data ORDER BY id";
        
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
            CadLogger::LogError(_T("插入用户数据失败: %s"), errorMsg.c_str());
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
            CadLogger::LogError(_T("插入项目数据失败: %s"), errorMsg.c_str());
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
            CadLogger::LogError(_T("插入图纸数据失败: %s"), errorMsg.c_str());
            return false;
        }
    }
    
    return true;
}

// 获取最后的错误信息
std::wstring SqlDB::getLastError()
{
    SQLWCHAR sqlState[6] = {0};
    SQLWCHAR messageText[1024] = {0};  // 增加缓冲区大小
    SQLINTEGER nativeError = 0;
    SQLSMALLINT textLength = 0;
    
    std::wstring result = L"未知错误";
    
    if (m_hDbc != SQL_NULL_HDBC) {
        SQLRETURN ret = SQLGetDiagRec(SQL_HANDLE_DBC, m_hDbc, 1, sqlState, &nativeError, 
                                     messageText, sizeof(messageText)/sizeof(SQLWCHAR), &textLength);
        
        if (ret == SQL_SUCCESS || ret == SQL_SUCCESS_WITH_INFO) {
            // 确保字符串正确终止
            if (textLength >= 0 && textLength < (sizeof(messageText)/sizeof(SQLWCHAR))) {
                messageText[textLength] = L'\0';
            } else {
                messageText[sizeof(messageText)/sizeof(SQLWCHAR) - 1] = L'\0';
            }
            
            result = L"SQL状态: " + std::wstring(sqlState) + 
                   L", 错误代码: " + std::to_wstring(nativeError) + 
                   L", 消息: " + std::wstring(messageText);
        } else {
            result = L"无法获取错误信息，SQLGetDiagRec返回码: " + std::to_wstring(ret);
        }
    }
    
    return result;
}

// 字符串转义函数
std::wstring escapeSqlString(const std::wstring& input)
{
    std::wstring result = input;
    size_t pos = 0;
    
    // 转义单引号
    while ((pos = result.find(L"'", pos)) != std::wstring::npos) {
        result.replace(pos, 1, L"''");
        pos += 2;
    }
    
    return result;
}

// 添加批量插入方法
bool SqlDB::executeBatchInsert(const std::wstring& tableName, 
                              const std::vector<std::wstring>& columns,
                              const std::vector<std::vector<std::wstring>>& values,
                              std::wstring& errorMsg)
{
    if (values.empty() || columns.empty()) {
        return true;
    }
    
    try {
        if (m_hDbc == SQL_NULL_HDBC) {
            errorMsg = L"数据库未连接";
            return false;
        }
        
        // 构建批量插入SQL
        std::wstring sql = L"INSERT INTO " + tableName + L" (";
        
        // 添加列名
        for (size_t i = 0; i < columns.size(); ++i) {
            if (i > 0) sql += L", ";
            sql += columns[i];
        }
        sql += L") VALUES ";
        
        // 添加值（分批处理，每批1000条）
        const size_t batchSize = 1000;
        for (size_t start = 0; start < values.size(); start += batchSize) {
          size_t end = (start + batchSize < values.size()) ? (start + batchSize) : values.size();
            
            std::wstring batchSql = sql;
            for (size_t i = start; i < end; ++i) {
                if (i > start) batchSql += L", ";
                batchSql += L"(";
                
                for (size_t j = 0; j < values[i].size(); ++j) {
                    if (j > 0) batchSql += L", ";
                    // 检查是否是数字或特殊函数
                    if (values[i][j] == L"NOW()") {
                        batchSql += values[i][j];
                    } else {
                        batchSql += L"'" + escapeSqlString(values[i][j]) + L"'";
                    }
                }
                batchSql += L")";
            }
            
            // 添加冲突处理
            batchSql += L" ON CONFLICT (file_path, entity_handle, text_content) DO UPDATE SET "
                       L"layer_name = EXCLUDED.layer_name, "
                       L"pos_x = EXCLUDED.pos_x, "
                       L"pos_y = EXCLUDED.pos_y, "
                       L"pos_z = EXCLUDED.pos_z, "
                       L"last_modified = EXCLUDED.last_modified";
            
            if (!executeQuery(batchSql, errorMsg)) {
                return false;
            }
            
            acutPrintf(_T("已处理 %d/%d 条记录\n"), (int)end, (int)values.size());
        }
        
        return true;
        
    } catch (...) {
        errorMsg = L"批量插入时发生异常";
        return false;
    }
}

// 专门用于文本索引的批量插入
bool SqlDB::batchInsertTextIndex(const std::vector<TextSearchResult>& textList, 
                                std::wstring& errorMsg)
{
    if (textList.empty()) {
        return true;
    }
    
    acutPrintf(_T("开始批量插入 %d 条文本索引记录...\n"), (int)textList.size());
    
    std::vector<std::wstring> columns = {
        L"file_path", L"text_content", L"layer_name", 
        L"pos_x", L"pos_y", L"pos_z", L"entity_handle", L"last_modified"
    };
    
    std::vector<std::vector<std::wstring>> values;
    values.reserve(textList.size());
    
    for (const auto& result : textList) {
        values.push_back({
            result.filePath,
            result.textContent,
            result.layerName,
            std::to_wstring(result.posX),
            std::to_wstring(result.posY),
            std::to_wstring(result.posZ),
            result.entityHandle,
            L"NOW()"
        });
    }
    
    bool success = executeBatchInsert(L"cad_text_index", columns, values, errorMsg);
    
    if (success) {
        acutPrintf(_T("批量插入完成！\n"));
    } else {
        acutPrintf(_T("批量插入失败: %s\n"), errorMsg.c_str());
    }
    
    return success;
} 



