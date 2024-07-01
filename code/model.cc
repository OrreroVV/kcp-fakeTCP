#ifndef  __MODEL_CPP__
#define __MODEL_CPP__

#include <bits/stdc++.h>

typedef uint32_t UI;

/*
sha256
*/

std::string getBin(int x) {
    std::string temp;
    while (x) {
        temp = char(x % 2 + '0') + temp;
        x /= 2;
    }
    return temp;
}

void addBin(std::string& res, int x) {
    std::string temp = getBin(x);
    while (temp.size() % 8 != 0) {
        temp = '0' + temp;
    }
    res += temp;
}

std::string SHA256_init(const std::string& ctx) {
    std::string res;
    for (size_t i = 0; i < ctx.size(); i++) {
        addBin(res, static_cast<int>(ctx[i]));
    }

    std::string temp = getBin(res.size());
    while (temp.size() < 64) {
        temp = '0' + temp;
    }
    assert(temp.size() <= 64);

    int k = ((448 - (res.size() + 1) % 512) % 512 + 512) % 512;
    res += '1';
    while (k--) {
        res += '0';
    }
    res += temp;
    assert(!(res.size() % 512));
    return res;
}

inline UI o1(UI x, UI y, UI z) {
    return (x & y) ^ (~x & z);
}

inline UI o2(UI x, UI y, UI z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

inline UI S(UI x, int n) {
    return (x >> n) | (x << (32 - n));
}

inline UI R(UI x, int n) {
    return x >> n;
}

inline UI o3(UI x) {
    return S(x, 2) ^ S(x, 13) ^ S(x, 22);
}

inline UI o4(UI x) {
    return S(x, 6) ^ S(x, 11) ^ S(x, 25);
}

inline UI o5(UI x) {
    return S(x, 7) ^ S(x, 18) ^ R(x, 3);
}

inline UI o6(UI x) {
    return S(x, 17) ^ S(x, 19) ^ R(x, 10);
}

const UI K[64] = {0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
   0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
   0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
   0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
   0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
   0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
   0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
   0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2};

std::string intToHex(int integerValue) {
    std::stringstream ss;
    ss << std::setfill('0') << std::setw(8) << std::hex << integerValue; 
    std::string hexString = ss.str();
    return hexString;
}

std::string SHA256_operate(std::string ctx, std::string& res16) {
    int N = ctx.size() / 512;
    std::vector<std::vector<UI>> H(N + 1, std::vector<UI>(8 + 1));
    H[0] = { 0, (UI)0x6a09e667, (UI)0xbb67ae85, (UI)0x3c6ef372, (UI)0xa54ff53a, (UI)0x510e527f,
     (UI)0x9b05688c, (UI)0x1f83d9ab, (UI)0x5be0cd19};
    for (int i = 1; i <= N; i++) {
        std::vector<UI> M(16), W(64);
        int base = (i - 1) * 512;
        for (int j = 0; j < 16; j++) {
            for (int k = 0; k < 32; k++) {
                M[j] = M[j] * 2 + ctx[base + j * 32 + k] - '0';
            }
        }

        for (int j = 0; j < 16; j++) W[j] = M[j];
        for (int j = 16; j < 64; j++) {
            W[j] = o6(W[j - 2]) + W[j - 7] + o5(W[j - 15]) + W[j - 16];
        }       

        UI a = H[i - 1][1], b = H[i - 1][2], c = H[i - 1][3], d = H[i - 1][4], e = H[i - 1][5], f = H[i - 1][6], g = H[i - 1][7], h = H[i - 1][8];
        for (int j = 0; j < 64; j ++) {
            UI t1 = h + o4(e) + o1(e, f, g) + K[j] + W[j];
            UI t2 = o3(a) + o2(a, b, c);
            h = g;
            g = f;
            f = e;
            e = d + t1;
            d = c;
            c = b;
            b = a;
            a = t1 + t2;
        }
        H[i] = { 0, a + H[i - 1][1], b + H[i - 1][2], c + H[i - 1][3], d + H[i - 1][4], e + H[i - 1][5], f + H[i - 1][6], g + H[i - 1][7], h + H[i - 1][8] };
    }  
    std::ostringstream oss;
    res16 = "";
    for (int i = 1; i <= 8; i++)
    {
         res16 += intToHex(H[N][i]);
         oss << std::setfill('0') << std::setw(32) << H[N][i];
    }
    return oss.str();
}

std::string encodePwdBySha256(const std::string& pwd, std::string& res16) {
    std::string ctx = SHA256_init(pwd);
    return SHA256_operate(ctx, res16);
}

/*
SM4算法
*/

UI sbox[] = { 0xD6, 0x90, 0xE9, 0xFE, 0xCC, 0xE1, 0x3D, 0xB7, 0x16, 0xB6, 0x14, 0xC2, 0x28, 0xFB, 0x2C, 0x05, 0x2B, 0x67, 0x9A,
                0x76, 0x2A, 0xBE, 0x04, 0xC3, 0xAA, 0x44, 0x13, 0x26, 0x49, 0x86, 0x06, 0x99, 0x9C, 0x42, 0x50, 0xF4, 0x91, 0xEF,
                0x98, 0x7A, 0x33, 0x54, 0x0B, 0x43, 0xED, 0xCF, 0xAC, 0x62, 0xE4, 0xB3, 0x1C, 0xA9, 0xC9, 0x08, 0xE8, 0x95, 0x80,
                0xDF, 0x94, 0xFA, 0x75, 0x8F, 0x3F, 0xA6, 0x47, 0x07, 0xA7, 0xFC, 0xF3, 0x73, 0x17, 0xBA, 0x83, 0x59, 0x3C, 0x19,
                0xE6, 0x85, 0x4F, 0xA8, 0x68, 0x6B, 0x81, 0xB2, 0x71, 0x64, 0xDA, 0x8B, 0xF8, 0xEB, 0x0F, 0x4B, 0x70, 0x56, 0x9D,
                0x35, 0x1E, 0x24, 0x0E, 0x5E, 0x63, 0x58, 0xD1, 0xA2, 0x25, 0x22, 0x7C, 0x3B, 0x01, 0x21, 0x78, 0x87, 0xD4, 0x00,
                0x46, 0x57, 0x9F, 0xD3, 0x27, 0x52, 0x4C, 0x36, 0x02, 0xE7, 0xA0, 0xC4, 0xC8, 0x9E, 0xEA, 0xBF, 0x8A, 0xD2, 0x40,
                0xC7, 0x38, 0xB5, 0xA3, 0xF7, 0xF2, 0xCE, 0xF9, 0x61, 0x15, 0xA1, 0xE0, 0xAE, 0x5D, 0xA4, 0x9B, 0x34, 0x1A, 0x55,
                0xAD, 0x93, 0x32, 0x30, 0xF5, 0x8C, 0xB1, 0xE3, 0x1D, 0xF6, 0xE2, 0x2E, 0x82, 0x66, 0xCA, 0x60, 0xC0, 0x29, 0x23,
                0xAB, 0x0D, 0x53, 0x4E, 0x6F, 0xD5, 0xDB, 0x37, 0x45, 0xDE, 0xFD, 0x8E, 0x2F, 0x03, 0xFF, 0x6A, 0x72, 0x6D, 0x6C,
                0x5B, 0x51, 0x8D, 0x1B, 0xAF, 0x92, 0xBB, 0xDD, 0xBC, 0x7F, 0x11, 0xD9, 0x5C, 0x41, 0x1F, 0x10, 0x5A, 0xD8, 0x0A,
                0xC1, 0x31, 0x88, 0xA5, 0xCD, 0x7B, 0xBD, 0x2D, 0x74, 0xD0, 0x12, 0xB8, 0xE5, 0xB4, 0xB0, 0x89, 0x69, 0x97, 0x4A,
                0x0C, 0x96, 0x77, 0x7E, 0x65, 0xB9, 0xF1, 0x09, 0xC5, 0x6E, 0xC6, 0x84, 0x18, 0xF0, 0x7D, 0xEC, 0x3A, 0xDC, 0x4D,
                0x20, 0x79, 0xEE, 0x5F, 0x3E, 0xD7, 0xCB, 0x39, 0x48};

const int keys[] = {-44, 99, 78, -12, -84, -124, -117, 79, -3, -40, -31, 116, 48, -125, -88, 72};

const UI CK[] = { 0x00070e15, 0x1c232a31, 0x383f464d, 0x545b6269,
                0x70777e85, 0x8c939aa1, 0xa8afb6bd, 0xc4cbd2d9,
                0xe0e7eef5, 0xfc030a11, 0x181f262d, 0x343b4249,
                0x50575e65, 0x6c737a81, 0x888f969d, 0xa4abb2b9,
                0xc0c7ced5, 0xdce3eaf1, 0xf8ff060d, 0x141b2229,
                0x30373e45, 0x4c535a61, 0x686f767d, 0x848b9299,
                0xa0a7aeb5, 0xbcc3cad1, 0xd8dfe6ed, 0xf4fb0209,
                0x10171e25, 0x2c333a41, 0x484f565d, 0x646b7279 };

const UI FK[] = { 0xa3b1bac6, 0x56aa3350, 0x677d9197, 0xb27022dc };

static std::vector<UI>MK(4);
static std::string key;
static std::vector<UI>rk(32);

UI LL(UI x, int n) {
    return x << n | x >> (32 - n);
}

UI L(UI x) {
    return x ^ LL(x, 2) ^ LL(x, 10) ^ LL(x, 18) ^ LL(x, 24);
}

UI L1(UI x) {
    return x ^ LL(x, 13) ^ LL(x, 23);   
}

UI Sbox(uint8_t a) {
    int x = 0, y = 0;
    x = (a & 0xf0) >> 4;
    y = (a & 0x0f);
    assert(x * 16 + y < 256);
    return sbox[x * 16 + y];
}

UI NT(UI x) {
    std::vector<UI>b(4);
    UI res = 0;
    for (int i = 0; i < 4; i++) {
        b[i] = Sbox((x >> (32 - 8 * (i + 1))) & 0xff);
        res = 16 * 16 * res + b[i];
    }
    return res;
}

UI T(UI x) {
    return L(NT(x));
}

UI T1(UI x) {
    return L1(NT(x));
}

std::string binaryStringToHex(std::string binaryString) {
    std::stringstream ss;
    std::bitset<8> bits;
    for (size_t i = 0; i < binaryString.length(); i += 8) {
        bits = std::bitset<8>(binaryString.substr(i, 8));
        ss << std::setfill('0') << std::setw(2) << std::hex << bits.to_ulong();
    }
    return ss.str();
}

/*
生成128位秘钥
*/

std::string toString8(int8_t x) {
    std::stringstream ss;

    ss <<  std::bitset<8>(x).to_string();
    return ss.str();
}

void initMK() {
    if (key.size())return;
    for (int i = 0; i < 16; i++) {
        key += toString8(keys[i]);
    }
    std::cout << "key: " << binaryStringToHex(key) << std::endl;
}
    
void initRK() {
    initMK();
    std::vector<UI>K(36);
    for (int i = 0; i < 4; i++) {
        MK[i] = std::stoul(key.substr(i * 32, 32), nullptr, 2);
        K[i] = MK[i] ^ FK[i];
    }   
    for (int i = 0; i < 32; i++) {
        rk[i] = K[i + 4] = K[i] ^ T1(K[i + 1] ^ K[i + 2] ^ K[i + 3] ^ CK[i]);
    }
}

UI F(UI x1, UI x2, UI x3, UI x4, UI x) {
    return x1 ^ T(x2 ^ x3 ^ x4 ^ x);
}



std::string sm4_128(std::string ctx, std::string IV) {
    initRK();
    std::vector<UI>X(36);

    try
    {
        for (int i = 0; i < 4; i++) {
            X[i] = std::stoul(ctx.substr(i * 32, 32), nullptr, 2) ^ std::stoul(IV.substr(i * 8, 8), nullptr, 16);
        }
    } catch(...)
    {
        std::cout << "xxxx\n";
    }
    
    for (int i = 0; i < 32; i++) {
        X[i + 4] = F(X[i], X[i + 1], X[i + 2], X[i + 3], rk[i]);
    }
    std::string res;
    for (int i = 0; i < 4; i++) {
        res += std::bitset<32>(X[35 - i]).to_string();
    }
    return binaryStringToHex(res);
}


void PKCS7(std::string& ctx) {
    if (ctx.size() % 16 == 0) {
        for (int i = 0; i < 16; i++) {
            ctx += "00010000";
        }
    } else {
        int padding = 16 - ctx.size() % 16;
        for (int i = 0; i < padding; i++) {
            ctx += std::to_string(padding);
        }
    }
}

/*
16进制输入
*/

std::string sm4(std::string ctx) {
    std::string temp = ctx;
    std::string xx;
    for (size_t i = 0; i < temp.size(); i++) {
        xx += std::bitset<8>((UI)temp[i]).to_string();
    }
    temp = xx;
    PKCS7(temp);

    ctx = temp;
    std::cout << "ctx: " << binaryStringToHex(ctx) << std::endl;

    std::string IV2 = "";

    for(size_t i = 0; i < 16; ++i) {

        IV2 += std::bitset<8>(i + 2).to_string();
    }   
    std::cout << "IV16: " << binaryStringToHex(IV2) << std::endl;
    std::string IV16 = binaryStringToHex(IV2);
    std::vector<std::string> blocks(ctx.size() / 128);
    for (size_t i = 0; i < ctx.size(); i += 128) {
        blocks.push_back(sm4_128(ctx.substr(i, 128), IV16));
        IV16 = blocks.back();
    }

    std::string res;
    for (auto i : blocks) {
        res += i; 
    }

   return res;
}

/*
base64加密
*/

const std::vector<char> base64_table { 'A', 'B', 'C', 'D', 'E', 'F', 'G',
    'H', 'I', 'J', 'K', 'L', 'M', 'N',
    'O', 'P', 'Q', 'R', 'S', 'T',
    'U', 'V', 'W', 'X', 'Y', 'Z',
    'a', 'b', 'c', 'd', 'e', 'f', 'g',
    'h', 'i', 'j', 'k', 'l', 'm', 'n',
    'o', 'p', 'q', 'r', 's', 't',
    'u', 'v', 'w', 'x', 'y', 'z',
    '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
    '+', '/' };

/*
16进制输入
*/

std::string base64_encode(const std::string& input) {
    std::string data;
    std::stringstream ss;
    
    for (auto i : input) {
        ss << std::bitset<4>(i >= 'A' ? i - 'A' + 10 : i - '0').to_string();
    }
    data = ss.str();
    while (data.size() % 6 != 0) {
        data += '0';
    }

    std::string res;
    for (size_t i = 0; i < data.size(); i += 6) {
        uint8_t byte = std::stoul(data.substr(i, 6), nullptr, 2);
        res += base64_table[byte];
    }
    if (data.size() % 24 != 0) {
        int gap = 24 - data.size() % 24;
        assert(gap % 6 == 0);
        while (gap) {
            res += '=';
            gap -= 6;
        }
    }

    return res;
}

/*
sm3
*/

int sm3_init(std::string& str) {
    int l = str.size();
    str += '1';
    int k = (448 - (l + 1) % 512 + 512) % 512;

    while (k--) {
        str += '0';
    }

    str += std::bitset<64>(l).to_string();
    assert(str.size() % 512 == 0);
    return str.size() / 512;
}

void sm3_init_B(const std::string& str, const int n, std::vector<std::vector<UI>>& B) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < 16; j++) {
            B[i][j] = std::stoul(str.substr(i * 512 + j * 32, 32), nullptr, 2);
        }
    }
}

UI FF(int j, UI x, UI y, UI z) {
    if (j <= 15) {
        return x ^ y ^ z;
    }

    return (x & y) | (x & z) | (y & z);
}

UI GG(int j, UI x, UI y, UI z) {
    if (j <= 15) {
        return x ^ y ^ z;
    }
    return (x & y) | (~x & z);
}

UI P0(UI x) {
    return x ^ LL(x, 9) ^ LL(x, 17);
}

UI P1(UI x) {
    return x ^ LL(x, 15) ^ LL(x, 23);
}

/*
二进制输入
15cd599de97b89e0bb5f0238fca481217b94616e05660403e176cdb1b5e6f230
15CD599DE97B89E0BB5F0238FCA481217B94616E05660403E176CDB1B5E6F230


BZ8un8FSpUvvHVXePx9dwXr7Owtr/Q1LBtMg0nulpQog2O7PewQMJHHliK/n7gQH7JIjQig9snUquPmPH9z6gKvw+F1YEJNncL52GNnIem0=
BZ8un8FSpUvvHVXePx9dwXr7Owtr/Q1LBtMg0nulpQog2O7PewQMJHHliK/n7gQH7JIjQig9snUquPmPH9z6gKvw+F1YEJNncL52GNnIem0=
*/
std::string sm3(const std::string& input) {
    std::string str;
    for (size_t i = 0; i < input.size(); i++) {
        str += std::bitset<8>((UI)input[i]).to_string();
    }

    // std::string tem;
    // str = "61626364616263646162636461626364616263646162636461626364616263646162636461626364616263646162636461626364616263646162636461626364";
    // for (int i = 0; i < str.size(); i ++) {
    //     tem += std::bitset<4>(str[i]).to_string();
    // }
    // str = tem;

    int n = sm3_init(str);

    std::vector<std::vector<UI>>B(n, std::vector<UI>(16, 0));
    std::vector<std::vector<UI>>V(n + 1, std::vector<UI>(8, 0));
    std::vector<UI> W(68), W1(64);

    V[0] = { (UI)0x7380166f, (UI)0x4914b2b9, (UI)0x172442d7, (UI)0xda8a0600, (UI)0xa96f30bc, (UI)0x163138aa, (UI)0xe38dee4d, (UI)0xb0fb0e4e};

    sm3_init_B(str, n, B);

    auto messageExtend = [B, &W, &W1](int i) -> void {
        for (size_t j = 0; j < 16; j++) {
            W[j] = B[i][j];
        }
        for (size_t j = 16; j < 68; j++) {
            W[j] = P1((W[j - 16] ^ W[j - 9] ^ LL(W[j - 3], 15))) ^ LL(W[j - 13], 7) ^ W[j - 6];
        }
        for (size_t j = 0; j < 64; j++) {
            W1[j] = W[j] ^ W[j + 4];
        }
    };

    auto sm3_T = [](int j) -> UI {
        return j >= 16 ? (UI)0x7a879d8a : (UI)0x79cc4519;
    };
    
    auto CF = [&sm3_T, &messageExtend, &W1, &W](int i, std::vector<UI> v) -> std::vector<UI> {
        messageExtend(i);
        UI A = v[0], B = v[1], C = v[2], D = v[3], E = v[4], F = v[5], G = v[6], H = v[7];
        for (int j = 0; j < 64; j++) {
            UI SS1 = LL((LL(A, 12) + E + LL(sm3_T(j), j)), 7); 
            UI SS2 = SS1 ^ LL(A, 12);
            UI TT1 = FF(j, A, B, C) + D + SS2 + W1[j];
            UI TT2 = GG(j, E, F, G) + H + SS1 + W[j];
            D = C;
            C = LL(B, 9);
            B = A;
            A = TT1;
            H = G;
            G = LL(F, 19);
            F = E;
            E = P0(TT2);
        }

        std::vector<UI> res(8);
        std::vector<UI> temp(8);
        temp = { A, B, C, D, E, F, G, H };
        for (int i = 0; i < 8; i++) {
            res[i] = temp[i] ^ v[i];
        }

        return res;
    };
    for (int i = 0; i < n; i++) {

        V[i + 1] = CF(i, V[i]);
    }
    std::string res;
    for (int i = 0; i < 8; i++) {
        res += binaryStringToHex(std::bitset<32>(V[n][i]).to_string());
    }
    return res;
}

std::string getEncryptPwd_sha256(std::string pwd) {
    std::string sha256_16;
    encodePwdBySha256(pwd, sha256_16);
    sha256_16 = "09117c03ccbdbc66bd49c0158df0ef24deda7808074e9c1b5d1306ddd3403c7c";
    std::string encryptData = sm4(sha256_16);
    std::string res = base64_encode(encryptData);

    std::cout << "sha256: " << sha256_16 << std::endl;
    std::cout << "sm4: " << encryptData << std::endl;
    std::cout << "base64: " << res << std::endl;

    return res;
}

std::string getEncryptPwd_sm3(std::string pwd) {
    std::string sm3_output;
    sm3_output = sm3(pwd);
    std::string encryptData = sm4(sm3_output);
    std::string res = base64_encode(encryptData);
    std::cout << "sm3: " << sm3_output << std::endl;
    std::cout << "sm4: " << encryptData << std::endl;
    std::cout << "base64: " << res << std::endl;
    return res;
}

#endif