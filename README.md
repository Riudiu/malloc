# Malloc 구현

## Main 파일 구성 요소

- **mm.{c,h}** : malloc 패키지를 직접 구현하는 파일

- mdriver.c : mm.c 파일을 테스트하는 malloc 드라이버

- short{1,2}-bal.rep : 시작하는 데 도움이 되는 두 개의 작은 추적 파일

- Makefile : driver를 빌드

## 드라이버에 대한 기타 지원 파일

- config.h : malloclab 드라이버를 구성
- fsecs.{c,h} :	다양한 타이머 패키지에 대한 wrapper 기능
- clock.{c,h} :	Pentium 및 Alpha 사이클 카운터에 액세스하기 위한 루틴
- fcyc.{c,h} : 사이클 카운터 기반 타이머 기능
- ftimer.{c,h} : 간격 타이머 및 get time of day()에 기반한 타이머 기능
- memlib.{c,h} : 힙 및 sbrk 함수 모델링

## 의도(Motivation)

- 지난 주에 사용했던 malloc을 이번 주에는 직접 만들어보자!
- 메모리 가상화 및 단편화, 할당 정책 등에 대한 이해도 상승
- 나만의 malloc, realloc, free 함수를 구현

## 테스트

- `make`후 `./mdriver -V` 실행하면 자세한 채점(테스트) 결과가 나옵니다. 

## 과제 설명

http://csapp.cs.cmu.edu/3e/malloclab.pdf   
출처: CMU (카네기멜론)

## CS:APP Malloc Lab

#### Copyright (c) 2002, R. Bryant and D. O'Hallaron, All rights reserved.
#### May not be used, modified, or copied without permission.