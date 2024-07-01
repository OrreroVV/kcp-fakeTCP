#include "code/model.cc"

using namespace std;

    
int main() {
    getEncryptPwd_sha256("5tgb^YHN");
    getEncryptPwd_sm3("5tgb^YHN");
    return 0;
    assert("YWRrPTEyMzA5MzI0JHNmc2Rsa2pzamFmbGRm" == base64_encode("adk=12309324$sfsdlkjsjafldf"));
    std::string res16;
    assert(sm4(res16) == "681EDF34D206965E86B3E94F536E4246");
    encodePwdBySha256("asdfadfasfsd", res16);
    assert(res16 == "7d4c691c129484aede0bc9b773b5f49257196940d980b5f04b7659b4c5cbcd17");
    encodePwdBySha256("123456", res16);
    std::cout << res16 << std::endl;
    assert(res16 == "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92");
}