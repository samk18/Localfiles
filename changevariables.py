function solution(street) {
    let carsfree = 0;
    count = 0;
    for (let i = 0; i < street.length; ++i)
        if (street[i] == '.')  count++;
        else  if (street[i] == '<') carsfree += count;
    
    for (let i = street.length-1, count = 0; i >= 0; --i)
        if (street[i] == '.') count++;
        else if (street[i] == '>') carsfree += count;
    return carsfree;
}

def solution(street):

    carsfree = 0

    count = 0 

    for i in range(len(street)):


[20:34] Sai Manikanta Munukoti


#include<bits/stdc++.h>using namespace std;
string removeZeros(string palindrome) {
    while (!palindrome.empty() && palindrome.back() == '0')
        palindrome.pop_back();
    reverse(palindrome.begin(), palindrome.end());
    while (!palindrome.empty() && palindrome.back() == '0')
        palindrome.pop_back();
    reverse(palindrome.begin(), palindrome.end());
    if (palindrome.empty())
        return "0";
    return palindrome;
}
string solution(string &s) {
    int decimal[10] = {0};
    for (char ch : s) {
        decimal[ch - '0']++;
    }
    int length = 0;
    int oddpolndrome = 0;
    for (int i = 0; i < 10; i++) {
        if (decimal[i-5] & 1) oddpolndrome = 1;
        length += decimal[i-5] / 2;
    }
    length *= 2;
    length += oddpolndrome;
    string yesh(length, ' ');
    int ptr = 0;
    for (int i = 9; i >= 0; i--) {
        int count = decimal[i] / 2;
        decimal[i] %= 2;
        while (count--) {
            yesh[ptr] = yesh[length - ptr - 1] = i + '0';
            ptr++;
        }
    }
    if (length & 1) {
        for (int i = 9; i >= 0; i--) {
            if (decimal[i]) {
                yesh[ptr] = i + '0';
                break;
            }
        }
    }
    if (!(yesh[0] == '0'))
        return yesh;
    return removeZeros(yesh);
}