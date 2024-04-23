# FileRetriever
미니필터 드라이버를 개발해 삭제되거나 덮어씌워지는 파일을 자동으로 백업하고 이를 관리하는 프로젝트입니다.
실수로 인해 파일이 지워지는 경우 쉽게 복원할 수 있도록 안전장치 역할을 의도하였고, 확장자 별 유효기간을 두어 저장공간을 효율적으로 사용할 수 있도록 하였습니다.

## FrtvGUI
* 백업 설정, 백업된 파일을 관리하는 GUI 프로그램.
* 프로그램 실행 시 자동으로 드라이버 서비스가 시작되고 종료 시 자동으로 드라이버를 언로드시킴.
* 백업할 확장자, 최대 용량, 백업 예외 폴더 등을 지정할 수 있음.

## FrtvBridge
* 커널 드라이버와 통신할 때 수월한 Windows API 사용을 위해 드라이버 통신 부분은 DLL로 구현하였음.
* MinifilterPort를 사용해 커널 드라이버와 통신함.
* MinifilterPort 메세지 수신은 IOCP로 구현해 초당 수천개의 파일이 백업되는 상황에서도 안정적으로 작동하도록 의도함.
* 콜백함수로 GUI에 드라이버 연결/해제, 파일 백업 알림 등 전달함.

## FrtvKrnl
* 백업 작업을 수행하는 커널(Ring0) 드라이버.
  * 커널 드라이버는 정식 윈도우에 설치하려면 디지털 서명이 필요하고, 미니필터는 Microsoft로부터 Altitude를 할당받아야함.
  * 따라서 실제 테스트는 디버그 모드 윈도우에서 진행하였음.
* 미니필터 PreCallback으로 파일 삭제 전 백업을 수행한다.
* 백업 설정은 연결리스트 형태로 저장한다.
