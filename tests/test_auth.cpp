#include <iostream>
#include <cassert>
#include <string>
#include <cstdio>
#include "auth/UserManager.h"

int main() {
    std::cout << "=== RUNNING AUTH UNIT TESTS ===" << std::endl;

    const std::string testStorage = "test_users_tmp.txt";
    std::remove(testStorage.c_str());

    UserManager mgr(testStorage);

    // Test 1: Register User
    std::string token1;
    UserAccount user1;
    std::string err;
    bool ok = mgr.registerUser("user_test", "123456", "Nguyen Van A", token1, user1, err);
    assert(ok && "Dang ky user_test thanh cong");
    assert(!token1.empty() && "Token khong duoc rong");
    assert(user1.username == "user_test");
    assert(user1.fullName == "Nguyen Van A");
    assert(!user1.passwordHash.empty());
    assert(mgr.userCount() == 1);
    std::cout << "[PASSED] Test 1: Dang ky user moi" << std::endl;

    // Test 2: Duplicate username
    std::string token2;
    UserAccount user2;
    ok = mgr.registerUser("user_test", "654321", "Nguoi Khac", token2, user2, err);
    assert(!ok && "Trung ten dang nhap phai that bai");
    assert(!err.empty());
    std::cout << "[PASSED] Test 2: Chan trung ten dang nhap" << std::endl;

    // Test 3: Short password
    ok = mgr.registerUser("user2", "123", "User 2", token2, user2, err);
    assert(!ok && "Mat khau < 6 ky tu phai that bai");
    std::cout << "[PASSED] Test 3: Chan mat khau ngan < 6 ky tu" << std::endl;

    // Test 4: Validate Token
    UserAccount validatedUser;
    ok = mgr.validateToken(token1, validatedUser);
    assert(ok && "Token hop le");
    assert(validatedUser.username == "user_test");
    std::cout << "[PASSED] Test 4: Kiem tra token hop le" << std::endl;

    // Test 5: Login with correct password
    std::string loginToken;
    UserAccount loginUser;
    ok = mgr.loginUser("user_test", "123456", loginToken, loginUser, err);
    assert(ok && "Dang nhap thanh cong");
    assert(!loginToken.empty());
    assert(loginUser.fullName == "Nguyen Van A");
    std::cout << "[PASSED] Test 5: Dang nhap dung mat khau" << std::endl;

    // Test 6: Login with wrong password
    ok = mgr.loginUser("user_test", "wrongpassword", loginToken, loginUser, err);
    assert(!ok && "Dang nhap sai mat khau phai that bai");
    std::cout << "[PASSED] Test 6: Tu choi sai mat khau" << std::endl;

    // Test 7: Logout
    ok = mgr.logout(token1);
    assert(ok && "Dang xuat thanh cong");
    ok = mgr.validateToken(token1, validatedUser);
    assert(!ok && "Token sau khi logout phai bi vo hieu hoa");
    std::cout << "[PASSED] Test 7: Dang xuat va huy session" << std::endl;

    // Test 8: Reload from disk file
    UserManager reloadedMgr(testStorage);
    assert(reloadedMgr.userCount() == 1);
    ok = reloadedMgr.loginUser("user_test", "123456", loginToken, loginUser, err);
    assert(ok && "Nap lai tai khoan tu file disk thanh cong");
    std::cout << "[PASSED] Test 8: Luu tru va phuc hoi tai khoan tu file disk" << std::endl;

    std::remove(testStorage.c_str());
    std::cout << "=== ALL AUTH UNIT TESTS PASSED SUCCESSFULLY! ===" << std::endl;
    return 0;
}
