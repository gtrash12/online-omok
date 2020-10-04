# online-omok
시스템프로그래밍 팀 프로젝트


프로젝트 목표
- 시스템 프로그래밍 특성을 최대한 활용 할 수 있는 프로그램
- 개발자 뿐만 아니라 일반 유저들에게도 실용성이 있는 프로그램
- 갖출 것은 다 갖춰 실제로 사용될 수 있는 높은 완성도의 프로그램

사용한 시스템 프로그래밍 특성
- 소켓 : 서로 다른 컴퓨터와의 통신
- 멀티스레딩 : Concurrent 한 오목 게임 제작
- 터미널컨트롤 : NON-CANONICAL MODE 이용한 실시간 키보드 입력 처리
- 뮤텍스 락 : 정확한 랭킹 기록
- 파일 시스템 : 파일에 랭킹 저장
- 시그날 : 컨트롤+c 감지해 처리

# 구현방법

로비와 방
- 로비는 select를 이용하여 구현해 방의 입장이나 랭킹 요청을 감지
- 대전이 성립되면 쓰레드를 생성하여 쓰레드가 두 플레이어의 대전을 처리하게 하고, select에서는 대전을 하는 두 유저의 소켓을 제거
- 쓰레드에서 처리하는 대전이 종료되면 진 쪽은 방에서 제외되고 양쪽 다 select에 다시 추가하여 방의 입장을 감지하게 함.
- 대전 중 한 쪽이 나가면 쓰레드가 소켓을 제거하고 방의 상태를 조정

오목 대전
- 서버에서는 쓰레드가 처리
- 클라이언트는 NONCANONICAL 모드로 전환해 실시간 키보드 입력을 좌표로 변환해 화면에 오목판을 출력해 줌
- 엔터를 누르면 해당 좌표를 서버로 전송시킴
- 서버에서 좌표를 받으면 그 좌표를 그대로 상대방 클라이언트로 보냄
- 상대방 클라이언트에서도 좌표를 기록해 오목판을 동기화 시킴
- 승패 여부는 서버에서 최근 놓여진 돌의 상하좌우 대각선을 순차적으로 탐색하며 결정

랭킹 시스템
- 대전에서 승리하면 랭킹에 등록
- 랭킹은 공유 데이터니 등록할 때는 뮤텍스 락을 얻어 진행
- 랭킹에 이미 동일한 이름이 랭크되어 있다면 그 점수를 넘을 때 까지 등록되지 않고, 점수를 넘으면 덮어씀
- 역순으로 비교하며 정렬
- 파일로 저장하고, 서버 오픈시 파일에서 읽어 와서 랭킹 초기화
