 # Unix-TermProject
윾닉스 텀프로젝트

## 1. 네이밍 규칙

### 1.1. 절대 줄여쓰지 않기
```c
// 매우 나쁜 예
char *strbuf;

// 아주 모범적인 예
char *stringBuffer;
```
***
### 1.2. 메서드 이름, 변수 이름: camelCase
```c
// 첫 단어는 소문자, 그 이후는 대문자 시작
public bool isGoodGame(char *gameName, int price)
{
  int index;
  int inputLength;
  bool isListEmpty;
  char *inputMessage;
  
  return false;
}
```

## 2. 괄호 스타일
https://ko.wikipedia.org/wiki/%EB%93%A4%EC%97%AC%EC%93%B0%EA%B8%B0_%EB%B0%A9%EC%8B%9D
우리는 BSD방식을 씁니다. ㅇㅇ;;
### 2.1. BSD 우리가 사용할 스타일
```c
// BSD
if (a == 1)
{
    for (int index = 0; index < 1000; index++)
    {
        if (b == 10)
        {
            처리();
        }
        else if(c == 10)
        {
            처리();
        }
    }
}
```
### 2.2. K&R (나쁜 예)
```c
// K&R
if(a == 1) {
    for (index = 0; index < 1000; index++) {
        if (b == 10) {
            처리();
        } else if(c == 10) {
            처리();
        }
    }
}
```

## 3. 지역변수 선언 위치
메서드 시작부분에 몰아 넣지 않기
```c
// GOOD :)
void kappa(char *input)
{
  int inputLength;
  inputLength = strlen(input);
  
  // do something
  
  char output[1024];
  for (int index = 0; index < inputLength; index++)
  {
    output[index] = input[index];
  }
  
  pid_t pid;
  switch (pid = fork())
  {
  }
} 
```
***
```c
// REALLY BAD!!!
void kappa(char *input)
{
  int inputLength;
  int index;
  char output[1024];
  pid_t pid;
  
  inputLength = strlen(input);
  
  // do something
  
  for (index = 0; index < inputLength; index++)
  {
    output[index] = input[index];
  }
  
  switch (pid = fork())
  {
  }
} 
```



## 4. 나머지
1. 탭 크기는 스페이스바 4번
2. 
