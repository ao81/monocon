//R08ものづくりコンテストシールドテスト
//入力回路はR07県大会の回路を使用
//GND:D4
//トグルスイッチ:D3
//タクトスイッチ:D2
//フォトインタラプタ:D1
//２軸ジョイスティック
//X軸,Y軸の読み取り:A1,A2

const int x_pin = A1; //X軸の出力をアナログA1へ接続。
const int y_pin = A2; //Y軸の出力をアナログA2へ接続。

const int D1 = 17; //フォトインタラプタをデジタルD1へ接続。
const int D2 = 18; //タクトスイッチをデジタルD2へ接続。
const int D3 = 19; //トグルスイッチをデジタルD3へ接続。
const int D4 = 20; //GNDに設定しD4へ接続。

const int M1 = 40; //M1（予備）へ接続。
const int M2 = 42; //M2（予備）へ接続。

int x_pos = 0; //初期状態設定。
int y_pos = 0;//初期状態設定。

int PHO = 0;
int SW = 0;
int TSW = 0;

float x_convert = 0.0f; //計算値の変数設定、オーバーフローしないようにflaot設定
float y_convert = 0.0f; //計算値の変数設定、オーバーフローしないようにflaot設定

void setup() {
 
  //----------------------------------------------------------------------------------------------------
  pinMode(20, OUTPUT);//IN_D4
  digitalWrite(20, LOW);//IN_D4をGNDと設定
  //----------------------------------------------------------------------------------------------------

  pinMode(A1, INPUT);//IN_A1
  pinMode(A2, INPUT);//IN_A2
  pinMode(D1, INPUT);//IN_D1
  pinMode(D2, INPUT);//IN_D2
  pinMode(D3, INPUT);//IN_D3
  pinMode(M1, INPUT);//IN_M1
  pinMode(M2, INPUT);//IN_M2

  Serial.begin(9600); //シリアル通信のデータ転送レートを9600bpsで指定。bpsはビット/秒。

}

void loop() {
x_pos = analogRead(x_pin); //X軸の読み出し
y_pos = analogRead(y_pin); //Y軸の読み出し
x_convert = (float)x_pos / 1023.0f * 5.0f; //読み取った値を電圧に変換、floatで計算
y_convert = (float)y_pos / 1023.0f * 5.0f; //読み取った値を電圧に変換、floatで計算

PHO = digitalRead(D1);
SW = digitalRead(D2);
TSW = digitalRead(D3);

// 以下シリアルモニタに表示
Serial.print("X / Voltage: ");
Serial.print(x_pos);
Serial.print(" / ");
Serial.print(x_convert);

Serial.print("     ");
Serial.print("Y / Voltage: ");
Serial.print(y_pos);
Serial.print(" / ");
Serial.print(y_convert);

Serial.print("     ");
if(!PHO == HIGH){
  Serial.print(" PHO = OFF ");
}else{
  Serial.print(" PHO = ON ");
}

Serial.print("     ");
if(SW == HIGH){
  Serial.print(" SW = OFF ");
}else{
  Serial.print(" SW = ON ");
}

Serial.print("     ");
if(!TSW == HIGH){
  Serial.print(" TSW = OFF ");
}else{
  Serial.print(" TSW = ON ");
}

Serial.println();

delay(1000); //1秒の遅延
}