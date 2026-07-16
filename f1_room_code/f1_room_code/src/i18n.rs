use crate::{AppWindow, State};
use slint::{ComponentHandle, SharedString};

pub fn configure_i18n_bindings(app: &AppWindow) {
    app.global::<State>().on_translate(|language, key| {
        SharedString::from(translate(language.as_str(), key.as_str()))
    });
}

fn translate<'a>(language: &str, key: &'a str) -> &'a str {
    let table = match language {
        "en" => EN,
        "vi" => VI,
        "es" => ES,
        "ja" => JA,
        "ar" => AR,
        _ => KO,
    };

    if let Some(value) = lookup(table, key).or_else(|| lookup(KO, key)) {
        value
    } else {
        key
    }
}

fn lookup(table: &'static str, key: &str) -> Option<&'static str> {
    for line in table.lines() {
        let (entry_key, value) = line.split_once('\t')?;
        if entry_key == key {
            return Some(value);
        }
    }
    None
}

const EN: &str = r####"absent_mode	Away Mode
absent_mode_desc	Automatic away-mode activation time
absent_mode_setting	Away Mode Time
app_settings	Settings
available_networks	Available Networks
auto_capture	Auto Capture
auto_capture_desc	Save visitor snapshots automatically
brightness	Brightness
call	Call
call_timeout_setting	Call Timeout
call_timeout_desc	No-answer ring timeout
call_time_setting	Call Time
call_time_desc	Maximum call duration
camera_playground_2	Camera 1 - Playground 2
cancel	Cancel
capture_records_desc	View captured screen images
capture_records_title	Capture Records
cctv	CCTV
clock_screen	Clock Screen
clock_screen_desc	Show clock on standby screen
color	Color
communication_room	Guard Room
confirm	Confirm
connect	Connect
connected	Connected
connection_failed	Connection Failed
connection_success	Connected
contrast	Contrast
current_password	Current Password
delete	Delete
delete_all	Delete All
delete_all_confirm	Delete all records?
delete_all_title	Delete All Records
delete_selected	Delete Selected
delete_selected_confirm	Delete the selected record?
delete_selected_title	Delete Selected Record
display_setting	Display Settings
door	Door
door_call_sound	Door Call Melody
door_call_title	Door Call
secondary_monitor_title	Secondary Monitor
door_open	Open Door
door_open_time	Door Open Time
door_open_time_desc	Auto lock delay
elevator	Elevator
elevator_calling	Calling the elevator...
end	End
enter_password	Enter Password
enter_password_placeholder	Enter password
etc_setting	Other Settings
factory_reset	Reset
factory_reset_desc	Restore all settings to factory defaults
general_setting	General Settings
guard_calling	Calling guard room...
guard_call_sound	Guard Call Melody
guard_room_call	Guard Room Call
individual_delete	Delete Selected
ip_address	IP Address
language_setting	Language Settings
lobby	Lobby
lobby_call_title	Lobby Entrance
mac_address	MAC Address
main_date	2026.03.27 Friday
mic_sensitivity	Mic Sensitivity
model_name	Model
mode_setting	Mode Settings
network	Network
new	NEW
new_password	New Password
online	ONLINE
password_change	Password Change
password_change_desc	Change door unlock password
password_changed	Changed
password_changed_desc	Password was changed successfully.
password_error	Password Error
password_error_desc	Current password does not match.
password_error_desc_2	Please try again.
password_input	Password Input
refresh	Refresh
reset	Reset
retry	Retry
ring_volume	Ring Volume
saturation	Saturation
save	Save
setting_info	Settings Info
setting_info_desc	Configure building and room number
building_number	Building
room_number	Room
enter_building_number	Enter building number
enter_room_number	Enter room number
setting_info_save_action	Save Settings Info
setting_info_saved	Saved
setting_info_saved_desc	Settings information was saved.
setting_info_save_failed	Save Failed
setting_info_save_failed_desc	Failed to save settings information.
sound_setting	Sound Settings
space	Space
system_info	System Info
system_info_desc	View device info and version
qr_unavailable	QR unavailable
system_reset	System Reset
system_reset_desc	All settings will be restored to factory defaults.
system_reset_desc_2	Do you want to continue?
system_version	System Version
talk_volume	Talk Volume
visitor_records	Visitor Records
wifi_on	On
app_version	App Version
captures	Captures
check_password	Check the password.
clear	Clear
clock_date	2026.03.30 Monday
display	Display
general	General
mode	Mode
other	Other
request	Request
sound	Sound
volume	Volume
wifi_off	Off
"####;

const VI: &str = r####"absent_mode	Che do vang nha
absent_mode_desc	Thoi gian kich hoat vang nha tu dong
absent_mode_setting	Thoi gian vang nha
app_settings	Cai dat
available_networks	Mang kha dung
auto_capture	Tu dong chup
auto_capture_desc	Luu anh khach tu dong
brightness	Do sang
call	Goi
call_timeout_setting	Thoi gian cho goi
call_timeout_desc	Tu dong ket thuc khi khong tra loi
call_time_setting	Thoi gian goi
call_time_desc	Thoi gian goi toi da
camera_playground_2	Camera 1 - San choi 2
cancel	Huy
capture_records_desc	Xem anh man hinh da chup
capture_records_title	Lich su chup
cctv	CCTV
clock_screen	Man hinh dong ho
clock_screen_desc	Hien dong ho khi cho
color	Mau sac
communication_room	Bao ve
confirm	Xac nhan
connect	Ket noi
connected	Da ket noi
connection_failed	Ket noi that bai
connection_success	Ket noi thanh cong
contrast	Tuong phan
current_password	Mat khau hien tai
delete	Xoa
delete_all	Xoa tat ca
delete_all_confirm	Ban co muon xoa tat ca ban ghi?
delete_all_title	Xoa tat ca ban ghi
delete_selected	Xoa muc da chon
delete_selected_confirm	Ban co muon xoa ban ghi da chon?
delete_selected_title	Xoa ban ghi da chon
display_setting	Cai dat man hinh
door	Cua
door_call_sound	Nhac goi cua
door_call_title	Goi cua
secondary_monitor_title	Giam sat may xac nhan phu
door_open	Mo cua
door_open_time	Thoi gian mo cua
door_open_time_desc	Thoi gian tu dong khoa
elevator	Thang may
elevator_calling	Dang goi thang may...
end	Ket thuc
enter_password	Nhap mat khau
enter_password_placeholder	Nhap mat khau
etc_setting	Cai dat khac
factory_reset	Khoi phuc
factory_reset_desc	Khoi phuc tat ca cai dat mac dinh
general_setting	Cai dat chung
guard_calling	Dang goi phong bao ve...
guard_call_sound	Nhac goi bao ve
guard_room_call	Goi phong bao ve
individual_delete	Xoa da chon
ip_address	Dia chi IP
language_setting	Cai dat ngon ngu
lobby	Sanh
lobby_call_title	Cua chung
mac_address	Dia chi MAC
main_date	2026.03.27 Thu sau
mic_sensitivity	Do nhay mic
model_name	Ten model
mode_setting	Cai dat che do
network	Mang
new	MOI
new_password	Mat khau moi
online	TRUC TUYEN
password_change	Doi mat khau
password_change_desc	Doi mat khau mo khoa cua
password_changed	Da doi
password_changed_desc	Mat khau da duoc doi thanh cong.
password_error	Loi mat khau
password_error_desc	Mat khau hien tai khong dung.
password_error_desc_2	Vui long kiem tra lai.
password_input	Nhap mat khau
refresh	Lam moi
reset	Dat lai
retry	Thu lai
ring_volume	Am luong chuong
saturation	Do bao hoa
save	Luu
setting_info	Thong tin cai dat
setting_info_desc	Cai dat toa nha va so phong
building_number	Toa nha
room_number	Phong
enter_building_number	Nhap so toa nha
enter_room_number	Nhap so phong
setting_info_save_action	Luu thong tin cai dat
setting_info_saved	Da luu
setting_info_saved_desc	Thong tin cai dat da duoc luu.
setting_info_save_failed	Luu that bai
setting_info_save_failed_desc	Khong the luu thong tin cai dat.
sound_setting	Cai dat am thanh
space	Khoang trang
system_info	Thong tin he thong
system_info_desc	Xem thong tin va phien ban thiet bi
qr_unavailable	Khong tao duoc QR
system_reset	Dat lai he thong
system_reset_desc	Tat ca cai dat se tro ve mac dinh.
system_reset_desc_2	Ban co muon tiep tuc?
system_version	Phien ban he thong
talk_volume	Am luong dam thoai
visitor_records	Lich su khach
wifi_on	Bat
app_version	Phien ban ung dung
captures	Ban chup
check_password	Vui long kiem tra mat khau.
clear	Xoa
clock_date	2026.03.30 Thu hai
display	Hien thi
general	Cai dat chung
mode	Che do
other	Khac
request	Goi
sound	Am thanh
volume	Am luong
wifi_off	Tat
"####;

const ES: &str = r####"absent_mode	Modo Ausente
absent_mode_desc	Hora de activacion automatica
absent_mode_setting	Tiempo en ausencia
app_settings	Configuracion
available_networks	Redes disponibles
auto_capture	Captura automatica
auto_capture_desc	Guardar fotos de visitantes automaticamente
brightness	Brillo
call	Llamar
call_timeout_setting	Espera de llamada
call_timeout_desc	Colgar si no contestan
call_time_setting	Tiempo de llamada
call_time_desc	Duracion maxima de llamada
camera_playground_2	Camara 1 - Parque 2
cancel	Cancelar
capture_records_desc	Ver imagenes capturadas
capture_records_title	Registros
cctv	CCTV
clock_screen	Pantalla de reloj
clock_screen_desc	Mostrar reloj en espera
color	Color
communication_room	Guardia
confirm	Confirmar
connect	Conectar
connected	Conectado
connection_failed	Conexion fallida
connection_success	Conexion correcta
contrast	Contraste
current_password	Contrasena actual
delete	Eliminar
delete_all	Eliminar todo
delete_all_confirm	Desea eliminar todos los registros?
delete_all_title	Eliminar todos los registros
delete_selected	Eliminar seleccionado
delete_selected_confirm	Desea eliminar el registro seleccionado?
delete_selected_title	Eliminar registro seleccionado
display_setting	Pantalla
door	Puerta
door_call_sound	Melodia de puerta
door_call_title	Llamada de puerta
secondary_monitor_title	Monitor secundario
door_open	Abrir puerta
door_open_time	Tiempo de apertura
door_open_time_desc	Tiempo de bloqueo automatico
elevator	Ascensor
elevator_calling	Llamando al ascensor...
end	Finalizar
enter_password	Introducir contrasena
enter_password_placeholder	Introduzca la contrasena
etc_setting	Otros ajustes
factory_reset	Restablecer
factory_reset_desc	Restaurar todos los ajustes de fabrica
general_setting	Ajustes generales
guard_calling	Llamando a la sala de guardia...
guard_call_sound	Melodia de guardia
guard_room_call	Llamada a guardia
individual_delete	Eliminar seleccionado
ip_address	Direccion IP
language_setting	Idioma
lobby	Lobby
lobby_call_title	Entrada comun
mac_address	Direccion MAC
main_date	2026.03.27 Viernes
mic_sensitivity	Sensibilidad del microfono
model_name	Modelo
mode_setting	Ajustes de modo
network	Red
new	NUEVO
new_password	Nueva contrasena
online	EN LINEA
password_change	Cambiar contrasena
password_change_desc	Cambiar la contrasena de apertura
password_changed	Cambio completado
password_changed_desc	La contrasena se cambio correctamente.
password_error	Error de contrasena
password_error_desc	La contrasena actual no coincide.
password_error_desc_2	Verifiquela de nuevo.
password_input	Entrada de contrasena
refresh	Actualizar
reset	Reiniciar
retry	Reintentar
ring_volume	Volumen del timbre
saturation	Saturacion
save	Guardar
setting_info	Info. de Ajustes
setting_info_desc	Configurar edificio y habitacion
building_number	Edificio
room_number	Habitacion
enter_building_number	Ingrese edificio
enter_room_number	Ingrese habitacion
setting_info_save_action	Guardar info. de ajustes
setting_info_saved	Guardado
setting_info_saved_desc	La informacion se guardo correctamente.
setting_info_save_failed	Error al Guardar
setting_info_save_failed_desc	No se pudo guardar la informacion.
sound_setting	Sonido
space	Espacio
system_info	Informacion del sistema
system_info_desc	Ver informacion y version del equipo
qr_unavailable	QR no disponible
system_reset	Restablecer sistema
system_reset_desc	Todos los ajustes volveran al estado de fabrica.
system_reset_desc_2	Desea continuar?
system_version	Version del sistema
talk_volume	Volumen de voz
visitor_records	Registros de visitas
wifi_on	Encendido
app_version	Version de la app
captures	Capturas
check_password	Revise la contrasena.
clear	Borrar
clock_date	2026.03.30 Lunes
display	Pantalla
general	General
mode	Modo
other	Otros
request	Llamar
sound	Sonido
volume	Volumen
wifi_off	Apagado
"####;

const JA: &str = r####"absent_mode	外出モード
absent_mode_desc	外出モード自動有効時間
absent_mode_setting	外出モード設定時間
app_settings	設定
available_networks	利用可能なネットワーク
auto_capture	自動映像キャプチャ
auto_capture_desc	来訪者を自動保存
brightness	明るさ
call	通話
call_timeout_setting	呼出タイムアウト
call_timeout_desc	未応答時に自動終了
call_time_setting	通話時間設定
call_time_desc	最大通話可能時間
camera_playground_2	カメラ1 - 公園2
cancel	キャンセル
capture_records_desc	キャプチャ画像を確認
capture_records_title	訪問者記録
cctv	CCTV
clock_screen	時計画面
clock_screen_desc	待機画面に時計を表示
color	色
communication_room	管理室
confirm	確認
connect	接続
connected	接続済み
connection_failed	接続失敗
connection_success	接続成功
contrast	コントラスト
current_password	既存パスワード
delete	削除
delete_all	全体削除
delete_all_confirm	すべての記録を削除しますか?
delete_all_title	全体記録削除
delete_selected	個別削除
delete_selected_confirm	選択した記録を削除しますか?
delete_selected_title	選択記録削除
display_setting	ディスプレイ設定
door	玄関
door_call_sound	玄関呼出音
door_call_title	玄関通話
secondary_monitor_title	二次確認機監視
door_open	ドア開放
door_open_time	ドア開閉時間
door_open_time_desc	自動ロック時間
elevator	エレベーター
elevator_calling	エレベーターを呼び出し中...
end	終了
enter_password	パスワード入力
enter_password_placeholder	パスワードを入力してください
etc_setting	その他設定
factory_reset	初期化
factory_reset_desc	すべての設定を工場出荷時に戻します
general_setting	一般設定
guard_calling	管理室を呼び出しています...
guard_call_sound	管理室呼出音
guard_room_call	管理室通話
individual_delete	個別削除
ip_address	IPアドレス
language_setting	言語設定
lobby	ロビー
lobby_call_title	共同玄関
mac_address	MACアドレス
main_date	2026.03.27 金曜日
mic_sensitivity	マイク感度
model_name	モデル名
mode_setting	モード設定
network	ネットワーク
new	NEW
new_password	新規パスワード
online	ONLINE
password_change	パスワード変更
password_change_desc	ドア開閉パスワード変更
password_changed	変更完了
password_changed_desc	パスワードが正常に変更されました。
password_error	パスワードエラー
password_error_desc	既存パスワードが一致しません。
password_error_desc_2	もう一度確認してください。
password_input	パスワード入力
refresh	更新
reset	初期化
retry	再試行
ring_volume	呼出音量
saturation	彩度
save	保存
setting_info	設定情報
setting_info_desc	棟番号と部屋番号を設定
building_number	棟番号
room_number	部屋番号
enter_building_number	棟番号を入力
enter_room_number	部屋番号を入力
setting_info_save_action	設定情報保存
setting_info_saved	保存完了
setting_info_saved_desc	設定情報を保存しました。
setting_info_save_failed	保存失敗
setting_info_save_failed_desc	設定情報を保存できませんでした。
sound_setting	サウンド設定
space	スペース
system_info	システム情報
system_info_desc	機器情報とバージョン確認
qr_unavailable	QRを生成できません
system_reset	システム初期化
system_reset_desc	すべての設定が工場出荷時に戻ります。
system_reset_desc_2	続行しますか?
system_version	システムバージョン
talk_volume	通話音量
visitor_records	訪問者記録
wifi_on	オン
app_version	アプリバージョン
captures	キャプチャ
check_password	パスワードを確認してください。
clear	クリア
clock_date	2026.03.30 月曜日
display	ディスプレイ
general	一般設定
mode	モード
other	その他
request	呼出
sound	サウンド
volume	音量
wifi_off	オフ
"####;

const AR: &str = r####"absent_mode	وضع الغياب
absent_mode_desc	وقت التفعيل التلقائي لوضع الغياب
absent_mode_setting	وقت وضع الغياب
app_settings	الإعدادات
available_networks	الشبكات المتاحة
auto_capture	التقاط تلقائي
auto_capture_desc	حفظ صور الزائر تلقائيا
brightness	السطوع
call	اتصال
call_timeout_setting	مهلة الاتصال
call_timeout_desc	إنهاء عند عدم الرد
call_time_setting	مدة الاتصال
call_time_desc	الحد الاقصى لمدة الاتصال
camera_playground_2	الكاميرا 1 - الملعب 2
cancel	إلغاء
capture_records_desc	عرض الصور الملتقطة
capture_records_title	سجل الزوار
cctv	CCTV
clock_screen	شاشة الساعة
clock_screen_desc	إظهار الساعة في وضع الانتظار
color	اللون
communication_room	غرفة الحراسة
confirm	تأكيد
connect	اتصال
connected	متصل
connection_failed	فشل الاتصال
connection_success	تم الاتصال
contrast	التباين
current_password	كلمة المرور الحالية
delete	حذف
delete_all	حذف الكل
delete_all_confirm	هل تريد حذف جميع السجلات؟
delete_all_title	حذف كل السجلات
delete_selected	حذف المحدد
delete_selected_confirm	هل تريد حذف السجل المحدد؟
delete_selected_title	حذف السجل المحدد
display_setting	إعدادات العرض
door	الباب
door_call_sound	نغمة نداء الباب
door_call_title	اتصال الباب
secondary_monitor_title	مراقبة جهاز التأكيد الثانوي
door_open	فتح الباب
door_open_time	مدة فتح الباب
door_open_time_desc	مدة القفل التلقائي
elevator	المصعد
elevator_calling	جار طلب المصعد...
end	إنهاء
enter_password	إدخال كلمة المرور
enter_password_placeholder	أدخل كلمة المرور
etc_setting	إعدادات أخرى
factory_reset	إعادة ضبط
factory_reset_desc	استعادة جميع الإعدادات الافتراضية
general_setting	الإعدادات العامة
guard_calling	جار الاتصال بغرفة الحراسة...
guard_call_sound	نغمة نداء الحراسة
guard_room_call	اتصال غرفة الحراسة
individual_delete	حذف المحدد
ip_address	عنوان IP
language_setting	إعداد اللغة
lobby	المدخل
lobby_call_title	المدخل المشترك
mac_address	عنوان MAC
main_date	2026.03.27 الجمعة
mic_sensitivity	حساسية الميكروفون
model_name	اسم الطراز
mode_setting	إعدادات الوضع
network	الشبكة
new	جديد
new_password	كلمة المرور الجديدة
online	متصل
password_change	تغيير كلمة المرور
password_change_desc	تغيير كلمة مرور فتح الباب
password_changed	تم التغيير
password_changed_desc	تم تغيير كلمة المرور بنجاح.
password_error	خطأ في كلمة المرور
password_error_desc	كلمة المرور الحالية غير مطابقة.
password_error_desc_2	يرجى التحقق مرة أخرى.
password_input	إدخال كلمة المرور
refresh	تحديث
reset	إعادة ضبط
retry	إعادة المحاولة
ring_volume	مستوى رنين الاتصال
saturation	التشبع
save	حفظ
setting_info	معلومات الإعداد
setting_info_desc	ضبط رقم المبنى والغرفة
building_number	المبنى
room_number	الغرفة
enter_building_number	أدخل رقم المبنى
enter_room_number	أدخل رقم الغرفة
setting_info_save_action	حفظ معلومات الإعداد
setting_info_saved	تم الحفظ
setting_info_saved_desc	تم حفظ معلومات الإعداد.
setting_info_save_failed	فشل الحفظ
setting_info_save_failed_desc	تعذر حفظ معلومات الإعداد.
sound_setting	إعدادات الصوت
space	مسافة
system_info	معلومات النظام
system_info_desc	عرض معلومات الجهاز والإصدار
qr_unavailable	تعذر إنشاء رمز QR
system_reset	إعادة ضبط النظام
system_reset_desc	ستتم استعادة جميع الإعدادات الافتراضية.
system_reset_desc_2	هل تريد المتابعة؟
system_version	إصدار النظام
talk_volume	مستوى صوت المكالمة
visitor_records	سجل الزوار
wifi_on	تشغيل
app_version	إصدار التطبيق
captures	اللقطات
check_password	يرجى التحقق من كلمة المرور.
clear	مسح
clock_date	2026.03.30 الاثنين
display	العرض
general	الإعدادات العامة
mode	الوضع
other	أخرى
request	طلب
sound	الصوت
volume	الصوت
wifi_off	إيقاف
"####;

const KO: &str = r####"absent_mode	외출모드
absent_mode_desc	외출모드 자동 활성화 시간
absent_mode_setting	외출모드 설정 시간
app_settings	설정
available_networks	사용 가능한 네트워크
auto_capture	자동 영상 캡쳐
auto_capture_desc	방문자 자동 촬영 저장
brightness	밝기
call	통화
call_timeout_setting	호출 대기 시간
call_timeout_desc	미응답 시 자동 종료
call_time_setting	통화 시간 설정
call_time_desc	최대 통화 가능 시간
camera_playground_2	카메라 1 - 놀이터 2 
cancel	취소
capture_records_desc	캡쳐된 화면 이미지 확인
capture_records_title	화면 저장 기록
cctv	CCTV
clock_screen	시계화면
clock_screen_desc	대기 화면에 시계 표시
color	색상
communication_room	경비실
confirm	확인
connect	연결
connected	연결됨
connection_failed	연결 실패
connection_success	연결 성공
contrast	명암
current_password	기존 비밀번호
delete	삭제
delete_all	전체 삭제
delete_all_confirm	모든 기록을 삭제하시겠습니까?
delete_all_title	전체 기록 삭제
delete_selected	개별 삭제
delete_selected_confirm	선택한 1개의 기록을 삭제하시겠습니까?
delete_selected_title	선택한 기록 삭제
display_setting	디스플레이 설정
door	현관
door_call_sound	현관 호출음
door_call_title	현관통화
secondary_monitor_title	2차 확인기 모니터링
door_open	문열림
door_open_time	도어 개폐 시간
door_open_time_desc	도어락 자동 잠금 시간
elevator	엘리베이터
elevator_calling	엘레베이터를 호출 중입니다...
end	종료
enter_password	비밀번호 입력
enter_password_placeholder	비밀번호를 입력하세요
etc_setting	기타설정
factory_reset	초기화
factory_reset_desc	모든 설정을 공장 초기값으로 복원
general_setting	일반설정
guard_calling	경비실을 호출 중입니다...
guard_call_sound	경비실 호출음
guard_room_call	경비실 통화
individual_delete	개별 삭제
ip_address	IP 주소
language_setting	언어 설정
lobby	로비
lobby_call_title	공동현관
mac_address	MAC 주소
main_date	2026.03.27 금요일
mic_sensitivity	마이크 감도
model_name	모델명
mode_setting	모드설정
network	네트워크
new	NEW
new_password	신규 비밀번호
online	ONLINE
password_change	비밀번호 변경
password_change_desc	도어 개폐 비밀번호 변경
password_changed	변경 완료
password_changed_desc	비밀번호가 성공적으로 변경되었습니다.
password_error	비밀번호 오류
password_error_desc	기존 비밀번호가 일치하지 않습니다.
password_error_desc_2	다시 확인해주세요.
password_input	비밀번호 입력
refresh	새로고침
reset	초기화
retry	다시 시도
ring_volume	호출음량
saturation	채도
save	저장
setting_info	설정 정보
setting_info_desc	동과 호수 정보를 설정
building_number	동
room_number	호수
enter_building_number	동 입력
enter_room_number	호수 입력
setting_info_save_action	세대 정보 저장
setting_info_saved	저장 완료
setting_info_saved_desc	설정 정보가 저장되었습니다.
setting_info_save_failed	저장 실패
setting_info_save_failed_desc	설정 정보를 저장하지 못했습니다.
sound_setting	사운드 설정
space	Space
system_info	시스템 정보
system_info_desc	기기 정보 및 버전 확인
qr_unavailable	QR 생성 불가
system_reset	시스템 초기화
system_reset_desc	모든 설정이 공장 초기값으로 되돌아갑니다.
system_reset_desc_2	계속하시겠습니까?
system_version	시스템 버전
talk_volume	통화음
visitor_records	방문자 기록
wifi_on	켜짐
app_version	어플 버전
captures	화면 저장 기록
check_password	비밀번호를 확인해주세요.
clear	지우기
clock_date	2026.03.30 월요일
display	디스플레이
general	일반설정
mode	모드설정
other	기타설정
request	호출
sound	사운드
volume	음량
wifi_off	꺼짐
"####;
