# Jenkins и стенд MCom03

## Резервирование

Jenkins:

    https://zeld-lpor-ci.elvees.com

Lockable resources имеют два независимых состояния:

- reserved: ручной резерв;
- locked: ресурс удерживает pipeline.

Не использовать ресурс, если установлен любой флаг. Несколько агентов работают под одним Jenkins-пользователем, поэтому reservedBy не различает агентов. Хранить список захваченных ресурсов в state-файле уникального worktree и освобождать только их.

Для пары MCom03 резервировать обе платы и связанные FPGA/auxiliary resources до первого SSH на hardware.

Не считать любые две свободные платы парой. Проверять физическую топологию. OpenWRT_ASSET-2815 был self-loop ресурсом на 2026-08-16; состояние и коммутацию перепроверять.

## Доступ к платам

После каждого reboot заново разрешать имя через DNS:

    ip=$(getent hosts OpenWRT_ASSET-7777 | awk 'NR==1 {print $1}')

DHCP-адреса могут меняться местами. Не сохранять старый IP как идентичность платы.

Безопасные SSH options с zeld:

    ssh -n -o BatchMode=yes       -o StrictHostKeyChecking=no       -o UserKnownHostsFile=/dev/null       root@"$ip" 'command'

Не использовать -n на принимающей стороне tar pipe: ей нужен stdin.

BusyBox на платах отличается от GNU tools:

- find не поддерживает -printf;
- сложные awk/regex могут отличаться;
- проверять PID через сохранённый pidfile и kill -0, а не pgrep -x;
- core1 и core2 обычно являются DSP-бинарниками, не core dumps.

## Запуск приложения

Использовать package launcher, если он настраивает design path и deployment profile. Для CSMACA:

    CSMACA_PHY=ofdm ./ue_start.sh 0
    CSMACA_PHY=dmr  ./ue_start.sh 0

Проверять фактический RUNTIME_DESIGN_PATH: прямой запуск cpu без него может незаметно выбрать design.yaml.

Умеренный logging обязателен для RF-графов. Уровень 10 способен создать непрерывный Backpressure QoS overflow и исказить результат.

TUN-проверка:

    ip route get PEER_IP
    ping -I ue ...
    cat /sys/class/net/ue/statistics/rx_packets
    cat /sys/class/net/ue/statistics/tx_packets

Не засчитывать ping только по 0% loss: конфликтующий локальный интерфейс может ответить сам себе. Требовать route через ue, реалистичный RTT и рост RX на противоположной плате.

## RF-диагностика

Порядок локализации:

1. Подтвердить живость host PID через kill -0.
2. Подтвердить отсутствие core.*.
3. Проверить design по именам блоков в runtime log.
4. Проверить RF sample rate, bandwidth, carrier и gain.
5. Проверить TX до RfSink и RX от RfSource.
6. Запустить контрольный DMR YAML на той же паре.
7. Запустить testRfLoop в соответствии с физической коммутацией.
8. Сравнить с точной baseline-версией, если regression неясен.

Не считать последовательный testRfLoop корректным для cross-linked пары: противоположная плата должна передавать одновременно. Для self-loop ресурса запускать один экземпляр.

## Очистка

Завершать приложения через TERM. Не использовать kill -9, кроме явно согласованной аварийной ситуации.

Перед освобождением:

    timeout 5 ssh ... root@"$ip" 'pkill -TERM -x cpu 2>/dev/null || true; reboot' || true

Reboot обрывает SSH, поэтому timeout/nonzero ожидаем. После этого освободить resources и явно проверить reserved=false, locked=false.
