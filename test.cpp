/*++

File Name:

    test.cpp

Abstract:

    Test numeric system for fun. 
        Numbers are in form XY-XY-...-XY-XY 
            where X is a set of letters (A-Z excluding some letters), 
            Y is a set of digits (0-9 excluding some digits)

Author:

    Maxim Lyadvinsky

Revision History:

    22/08/2022 - Maxim Lyadvinsky - Created

--*/

#include <string>
#include <mutex>
#include <iostream>

namespace uniteller_test {

class BasicDigit
{
public:
    BasicDigit() : overflow_(false) {}
    BasicDigit(const BasicDigit& d) : overflow_(d.overflow_) {}
    virtual bool IsValid() const = 0;
    virtual void SetMinDigit() = 0;
    virtual bool IsMaxDigit() const = 0;
    virtual std::string Id() const = 0;
    virtual void Lock() const {}
    virtual void Unlock() const {}
    bool IsOverflow() const { return overflow_; }
    void SetOverflow(bool set = true) { overflow_ = set; }
    void ResetOverflow() { SetOverflow(false); }
    BasicDigit& operator++()
    {
        Lock();
        do {
            if (IsMaxDigit()) {
                SetMinDigit();
                SetOverflow();
            } else {
                IncRawValue();
            }
        } while (!IsValid());
        Unlock();
        return *this;
    }

protected:
    virtual void IncRawValue() = 0;

private:
    bool overflow_;
};


class DigitC : public BasicDigit
{
public:
    DigitC(char v) : v_(v) {}
    virtual std::string Id() const
    {
        char buf[2];
        buf[0] = v_;
        buf[1] = 0;
        return std::string(buf);
    }

protected:
    virtual void IncRawValue() { ++v_; };
    char v_;
};


// A, B, C, !D, E, !F, !G, H, I, !J, K, L, !M, N, O, P, !Q, R, S, T, U, !V, W, X, Y, Z
class DigitH : public DigitC
{
public:
    DigitH() : DigitC('A') {}
    DigitH(char v) : DigitC(v) {}
    virtual bool IsValid() const
    {
        return (v_ >= 'A' && v_ < 'D') ||
            (v_ == 'E') ||
            (v_ > 'G' && v_ < 'J') ||
            (v_ > 'J' && v_ < 'M') ||
            (v_ > 'M' && v_ < 'Q') ||
            (v_ > 'Q' && v_ < 'V') ||
            (v_ > 'V' && v_ <= 'Z');
    }
    virtual void SetMinDigit() { v_ = 'A'; }
    virtual bool IsMaxDigit() const { return v_ == 'Z'; }
};


// 1 - 9
class DigitL : public DigitC
{
public:
    DigitL() : DigitC('1') {}
    DigitL(char v) : DigitC(v) {}
    virtual bool IsValid() const { return v_ > '0' && v_ <= '9'; }
    virtual void SetMinDigit() { v_ = '1'; }
    virtual bool IsMaxDigit() const { return v_ == '9'; }
};


// Digits A1 - Z9
class Digit : public BasicDigit
{
public:
    Digit() {}
    Digit(const char* id) : h_(id ? id[0] : 0), l_(id ? id[1] : 0) {}
    virtual bool IsValid() const { return h_.IsValid() && l_.IsValid(); }
    virtual void SetMinDigit() { h_.SetMinDigit(); l_.SetMinDigit(); }
    virtual bool IsMaxDigit() const { return h_.IsMaxDigit() && l_.IsMaxDigit(); }
    virtual std::string Id() const { return h_.Id() + l_.Id(); }

protected:
    virtual void IncRawValue()
    {
        ++l_;
        if (l_.IsOverflow()) {
            l_.ResetOverflow();
            ++h_;
            if (h_.IsOverflow())
                SetOverflow();
        }
    };

private:
    DigitH h_;
    DigitL l_;
};


class Number : public BasicDigit
{
public:
    static const int max_digits = 10;
    Number() : digits_(1) {}
    Number(const char* id) : digits_(1)
    {
        Digit digit[max_digits];
        int i = 0;
        bool valid = true;
        while (id && id[0])
        {
            Digit d(id);
            if (d.IsValid()) {
                digit[i] = d;
                i++;
                id += 2;
                if (id[0] == '-')
                    id++;
                else
                    break;
            } else {
                valid = false;
                break;
            }
        }
        if (valid) {
            digits_ = i;
            for (i = 0; i < digits_; i++)
            {
                digit_[i] = digit[digits_ - i - 1];
            }
        }
    }
    virtual bool IsValid() const
    {
        bool valid = true;
        Lock();
        for (int i = 0; i < digits_; i++) {
            if (!digit_[i].IsValid()) {
                valid = false;
                break;
            }
        }
        Unlock();
        return valid;
    }
    virtual void SetMinDigit()
    {
        Lock();
        digit_[0].SetMinDigit();
        digits_ = 1;
        Unlock();
    }
    virtual bool IsMaxDigit() const
    {
        Lock();
        bool result = digits_ == max_digits && digit_[digits_ - 1].IsMaxDigit();
        Unlock();
        return result;
    }
    virtual std::string Id() const
    {
        std::string id;
        Lock();
        for (int i = digits_; i > 0; i--)
        {
            id += digit_[i - 1].Id();
            if (i > 1)
                id += "-";
        }
        Unlock();
        return id;
    }
protected:
    virtual void IncRawValue() {
        for (int i = 0; i < digits_; i++)
        {
            ++digit_[i];
            if (digit_[i].IsOverflow()) {
                digit_[i].ResetOverflow();
                if (i == digits_ - 1) {
                    if (digits_ == max_digits) {
                        SetOverflow();
                        digits_ = 1;
                        digit_[0].SetMinDigit();
                    } else {
                        ++digits_;
                        digit_[digits_ - 1].SetMinDigit();
                    }
                    break;
                }
            } else {
                break;
            }
        }
    }
    virtual void Lock() const { sync_.lock(); }
    virtual void Unlock() const { sync_.unlock(); }

private:
    Digit digit_[max_digits];
    int digits_;
    mutable std::recursive_mutex sync_;
};

}

int main(int argc, char* argv[])
{
    uniteller_test::Number num(argc > 0 ? argv[1] : "");
    std::cout << num.Id().c_str() << std::endl;
    ++num;
    std::cout << num.Id().c_str() << std::endl;
    return 0;
}
