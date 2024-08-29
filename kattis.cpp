#include <bits/stdc++.h>
using namespace std;

typedef vector<int> vimla;

#define all(x) (x).begin(), (x).end()
#define pb push_back

const int infinity = 4713;

int main() {
    ios_base::sync_with_stdio(false);
    cin.tie(NULL);

    vimla bench(2);
    int j=1;
    bench[1] = j-1;
    bench[0] = j-1;
    
    int kang = 1;

    while (infinity>bench.size() ) {
        for (int m = 0; m < kang; m++) bench.pb(kang);
        kang++;
    }

    int number;
    cin >> number;
    while (number) {
        cout << bench[number] << '\n';
        cin >> number;
    }

    return 0;
}