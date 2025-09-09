#pragma once

// 解决编码警告
#pragma warning(disable: 4819)

#include <string>
#include <vector>
#include <windows.h>
#include <sql.h>
#include <sqlext.h>

// 重命名为DbSheetData避免冲突
struct DbSheetData {
    int id;
    std::wstring name;
    std::wstring buildingName;
    std::wstring specialty;
    std::wstring format;
    std::wstring status;
    std::wstring version;
    std::wstring designUnit;
    std::wstring createTime;
    std::wstring creator;
    bool isSelected;
    std::wstring filePath;  // 记录文件路径，用于后续文件上传到服务器
};

class SqlDB
{
public:
    SqlDB();
    ~SqlDB();

    // 数据库连接管理
    static bool initDatabase();
    static bool testDatabaseConnection();
    static void closeDatabase();
    static bool isDatabaseOpen();
    
    // 测试数据管理
    static bool createTestTables();
    static bool insertTestData();
    static bool showTestData();
    static bool queryTestData();
    static bool clearTestData();
    
    // 图纸数据管理
    // 创建指定名称和结构的表
    static bool createSheetTable(const std::wstring& tableName, const std::wstring& tableStructure);
    static bool saveSheetData(const std::vector<DbSheetData>& sheetList, std::wstring& errorMsg);
    static bool loadSheetData(std::vector<DbSheetData>& sheetList, std::wstring& errorMsg);
    static bool clearSheetData(std::wstring& errorMsg);
    
    // 通用数据库操作
    static bool executeQuery(const std::wstring& sql, std::wstring& errorMsg);
    static bool executeQuery(const std::wstring& sql);
    
    // 字段管理方法（支持指定表名）
    static bool addFieldToTable(const std::wstring& tableName, const std::wstring& fieldName, const std::wstring& fieldType, const std::wstring& defaultValue = L"");
    static bool removeFieldFromTable(const std::wstring& tableName, const std::wstring& fieldName);
    static bool fieldExists(const std::wstring& tableName, const std::wstring& fieldName);
    static std::vector<std::wstring> getTableFields(const std::wstring& tableName);
   
private:
    static SQLHENV m_hEnv;      // 环境句柄
    static SQLHDBC m_hDbc;      // 连接句柄
    
    // 私有辅助方法
    static bool createUserTable();
    static bool createProjectTable();
    static bool createDrawingTable();
    static bool insertUserData();
    static bool insertProjectData();
    static bool insertDrawingData();
    static std::wstring getLastError();
};
