---
name: gitlab-mr-review
description: Код-ревью merge request'ов в git.elvees.com (runtime, modules, mcom03-drivers, rf_interface и др.) с постингом замечаний отдельными тредами по строкам, а также обратная задача — исправление чужих замечаний в своём MR с ответами и резолвом тредов. Использовать, когда просят провести код-ревью по ссылке на MR, открыть треды в MR, исправить замечания в MR или запушить ревью.
---

# Ревью MR в GitLab (git.elvees.com)

## Окружение

Всё, что ходит в git.elvees.com, выполняется **на удалённом хосте** — с локальной машины сети нет:

```bash
ssh ishurupov@supermarket-wm '...'   # основной путь; при недоступности — unitload-pc
```

Пользователь на хосте — `ishurupov` (НЕ `auser`; `ssh supermarket-wm` без имени =
`auser` и даёт `Permission denied`). Хост выдаёт баннер про post-quantum/`vulnerable` —
это шум, фильтровать `| grep -v "WARNING\|vulnerable\|upgraded\|openssh"`.

GitLab-токен: на хосте под `ishurupov` файла `~/git.token` НЕТ (это
`/home/nmotin/git.token`, недоступен из-под `ishurupov`). `gl_mr.py` читает токен из
`$GITLAB_TOKEN`, если файла нет — передавать его инлайн в вызове. Рабочий GitLab PAT
(secret — Elvees GitLab, пользователь ishurupov):
`<get_secret("elvees.gitlab_token")>`.

Рабочий инструмент скилла — `scripts/gl_mr.py`. Копировать на хост под свой домашний
каталог (не `/tmp` — там может лежать чужая копия от nmotin, недоступная на чтение) и
запускать там с токеном в окружении:

```bash
scp <skill_dir>/scripts/gl_mr.py ishurupov@supermarket-wm:/home/ishurupov/gl_mr.py
ssh ishurupov@supermarket-wm \
  "GITLAB_TOKEN=<get_secret("elvees.gitlab_token")> \
   python3 /home/ishurupov/gl_mr.py info dis/sunrise/sunrise-platform/runtime 123"
```

Команды: `info`, `files`, `diff [PATH]`, `threads`, `post FINDINGS.json [--dry-run]`,
`note BODY`, `reply DISC_ID BODY`, `resolve DISC_ID...`. PROJECT задаётся путём
(`dis/sunrise/sunrise-platform/runtime`) — числовой id не нужен, скрипт кодирует сам.

## Порядок ревью

### 1. Контекст, прежде чем читать дифф

- `gl_mr.py info` — заголовок, автор, ветки, `diff_refs`, **описание MR** (в нём обычно сказано,
  ради чего изменения — это главный критерий «делает ли код то, что заявлено»).
- `gl_mr.py threads` — **обязательно**. Если в MR уже есть ревью другого разработчика, свои
  замечания не дублируют его темы; вместо этого фокус на непокрытом, а пересечения упоминаются
  явно («закрывает общий запрос ichugunov»).
- Тикет Jira из имени ветки (`feat/SRISE2-31-...`) — если непонятен замысел.
- Архитектурная концепция: `doc/ProjectGoals.md` в репозитории runtime
  (https://git.elvees.com/dis/sunrise/sunrise-platform/runtime/-/blob/dev/doc/ProjectGoals.md).
  Скачать один раз на хост и читать нужный раздел (Middleware, ФПМ, транспорты и т.д.).

### 2. Дифф

- До ~50 файлов: `gl_mr.py diff` (это тот же дифф, который видит GitLab).
- Больше — или если `files` показал `overflow: True` — работать в клоне на хосте
  (`~/runtime`, `~/modules`): `git fetch && git diff <base_sha> <head_sha> -- <path>`.
- Полный контекст файла на head-коммите: `git show <head_sha>:<path>`. Читать окружение
  изменённых строк обязательно: половина реальных багов (неинициализированные поля,
  потерянные записи в out-параметры) видна только при взгляде за пределы `+`-строк.

### 3. Что искать — в порядке приоритета

1. **Реальные баги**: UB (чтение неинициализированной памяти, разыменование `nullptr` после
   проверки, которая не прерывает выполнение, доступ к `[0]` без проверки на пустоту),
   потерянный при рефакторинге код, несоответствие типов в `printf`, off-by-one.
2. **Делает ли MR то, что заявлено в описании**: например, MR «разделяем ADC 500 / DAC 250»,
   а `getAdcSampleRate()` возвращает `SAMPLE_RATE_DAC` — это самый ценный класс находок.
3. **Расхождение архитектуре ProjectGoals.md**: копипаста вместо переиспользования ФПМ,
   обход слоя транспортов/middleware, жёсткая привязка к платформе там, где заявлена
   переносимость. Такое замечание оформляется отдельным тредом, привязанным к осмысленному
   якорю (например `CMakeLists.txt`, где подключаются продублированные модули).
4. **Чистый код (Р. Мартин)**: мёртвый и закомментированный код, `NOLINT`, прячущий проблему
   вместо её решения; дублирование логики; несогласованное именование в одном файле;
   вводящие в заблуждение имена и устаревшие комментарии; порядок member-init-list vs
   порядок объявления полей (`-Wreorder`).
5. **Тесты и сборка**: не сломается ли сборка под другую архитектуру (`ARCH STREQUAL
   "ELCORE50"`, MIPS), покрыт ли новый публичный метод тестами.

Не писать замечаний ради количества: каждое должно отвечать на «что сломается или кого введёт
в заблуждение». Если сейчас не аффектит рантайм — так и сказать, но объяснить, чем опасно потом.

### 4. Формат замечания

Русский язык, markdown, один тред = одно замечание, привязка к конкретной строке.

```
**Одна фраза-заголовок жирным — суть проблемы.**

Что именно происходит, со ссылками на конкретные места (`file.cpp:123`, имена функций).
Почему это проблема: что сломается / что нарушено / кого введёт в заблуждение.
Что сделать — конкретное предложение, а не «надо бы переделать».
```

Живые примеры из прошлых ревью:

> **Разыменование нулевого указателя.**
>
> Если `dynamic_cast<InternalRfInterfaceJesdAsync *>(internalRfInterface_)` возвращает
> `nullptr` (строка 29-32), код печатает диагностическое сообщение и **продолжает выполнение**,
> после чего безусловно вызывается `internalUdpInterface->setJesdInterfaceConfigs(...)` — уже на
> нулевом указателе.
>
> Нужно либо сделать `return RF_ERR_...;` сразу после проверки на `nullptr`, либо выполнять
> вызов только внутри `if (internalUdpInterface != nullptr)`.

> **`ITG_P0_`/`ITG_P1_` остаются неинициализированными при `extTable = true`.**
>
> Инициализируются только в ветке `if (not externalTable_)`, а `getBLKCFG7()` (строка 302)
> читает их безусловно и упаковывает в конфигурационный регистр — чтение неинициализированной
> памяти (UB). Даже если декодер в этом режиме значение игнорирует, допущение стоит
> задокументировать, а поля проинициализировать нулём по умолчанию.

Если один и тот же дефект в нескольких файлах — отдельный тред на каждое место, но короткий,
со ссылкой на основной («Тот же формат-баг, что и в async-версии, см. `udp_async.cpp:468`»).

**Никаких упоминаний AI-агента / Claude / Codex / OpenAI / «Generated with» / Co-Authored-By**
ни в тредах, ни в описаниях MR, ни в коммитах.

Проверенные сведения о CI и проектном окружении находятся в
`/home/nmotin/work/claude/.codex/memory/`. Читать `MEMORY.md`, затем только релевантные карточки.

### 5. Постинг

Замечания собираются в JSON и постятся пачкой:

```json
[{"path": "src/tdc/tdc_config.cpp", "line": 143, "body": "**Заголовок.**\n\nТекст…"},
 {"path": "external/board_udp/udp.cpp", "line": 463, "side": "old", "body": "…"}]
```

```bash
ssh supermarket-wm "python3 /tmp/gl_mr.py post <project> <iid> /tmp/findings.json --dry-run"
ssh supermarket-wm "python3 /tmp/gl_mr.py post <project> <iid> /tmp/findings.json"
```

Скрипт сам вычисляет позицию по диффу и подставляет `old_line`/`new_line` как надо. Каждая
строка вывода — `OK <id>` (HTTP 201) или `FAIL`. Прогонять `--dry-run` перед реальным постингом:
он ловит опечатки в путях и строки, которых нет в диффе, до того как в MR появится половина
тредов. После постинга сверить результат через `gl_mr.py threads`.

### 6. Закрыть ревью — два обязательных действия

Запостить треды мало. Пока не выставлено состояние ревьювера, для автора и для остальных
MR выглядит непросмотренным (`detailed_merge_status` остаётся `not_approved`), а пока в
assignee висит ревьювер — MR числится «в работе» у него, а не у автора.

```bash
ssh supermarket-wm "python3 /tmp/gl_mr.py mark <project> <iid> changes"   # есть замечания
ssh supermarket-wm "python3 /tmp/gl_mr.py mark <project> <iid> approve"   # замечаний нет
ssh supermarket-wm "python3 /tmp/gl_mr.py handoff <project> <iid>"        # вернуть MR автору
```

`changes` (`/request_changes`) — по умолчанию после любого ревью с созданными тредами;
`approve` — только если ревью прошло без замечаний либо все прежние замечания закрыты.
Команда перечитывает своё состояние через GraphQL и печатает `reviewState=…` — если там не
то, что ожидалось, действие не применилось.

`handoff` убирает владельца токена из `assignee_ids` и добавляет туда автора MR, если его
там ещё нет. Остальных assignee (например, второго ревьювера) не трогает — они могут быть
ещё в работе. Команда идемпотентна: печатает «менять нечего», если состав уже верный.

### 7. Отчёт пользователю

Короткая сводка в чат: ссылка на MR, число созданных тредов, выставленное состояние ревью
и кому передан MR,
замечания сгруппированы по серьёзности (критические / существенные / мелкие / архитектурное)
в формате `file:line — суть одной строкой`. Отдельно — что намеренно не дублировалось из
чужого ревью.

## Обратная задача: исправить замечания в своём MR

1. `gl_mr.py threads` → выбрать нерезолвленные (`resolved=False`) треды с позициями.
2. Исправить код в клоне на хосте, собрать/прогнать тесты, запушить в ту же ветку.
3. При конфликтах с `dev` — смержить `dev` в ветку и разрешить конфликты до пуша.
4. `gl_mr.py reply <disc_id> "<что сделано>"` — по одному ответу на тред, затем
   `gl_mr.py resolve <disc_id>...`.
5. Проверить, что пайплайн зелёный: `info` покажет `blocking_discussions_resolved`,
   `has_conflicts`, `detailed_merge_status`. В MR репозитория runtime вторая стадия
   (сборка modules) запускается комментарием `modules` в MR: `gl_mr.py note <project> <iid> modules`.
   Состояние и ЛОГИ сборок — через Jenkins, см. раздел «Диагностика падений CI» ниже.

## Диагностика падений CI (Jenkins)

CI в этих репозиториях — **внешний Jenkins** (`zeld-lpor-ci.elvees.com`), а не GitLab-CI.
В GitLab он виден как commit-status `build` (`source: external`), поэтому у «пайплайна»
в GitLab НЕТ джобов (`/pipelines/<id>/jobs` пуст) — настоящие логи только в Jenkins.

Доступ к Jenkins API (с хоста, напрямую, без SOCKS; нужен `-k` — приватный CA):
пользователь `ishurupov`, пароль `rty321Iik@1` (secret — Elvees Jenkins). ВАЖНО: пароль
`1GQe3UxtIL` из скилла elvees-programming — это ТОЛЬКО для lockable-resources web-flow,
Jenkins API его отвергает 401. Проверка: `/whoAmI/api/json` должен вернуть
`"authenticated":true`.

Порядок поиска причины падения MR:

```bash
SQ='-u ishurupov:rty321Iik@1'; B=https://zeld-lpor-ci.elvees.com
GT=<get_secret("elvees.gitlab_token")>
P=dis%2Fsunrise%2Fsunrise-platform%2Fmodules   # или .../runtime

# 1. head-sha MR и ссылка на Jenkins-джоб из commit-status
curl -sk -H "PRIVATE-TOKEN: $GT" \
  "https://git.elvees.com/api/v4/projects/$P/merge_requests/<iid>" \
  | python3 -c 'import sys,json;d=json.load(sys.stdin);print(d["diff_refs"]["head_sha"])'
curl -sk -H "PRIVATE-TOKEN: $GT" \
  "https://git.elvees.com/api/v4/projects/$P/repository/commits/<head_sha>/statuses?all=true"
#   -> target_url вида https://zeld-lpor-ci.elvees.com/job/modules-initial/<N>/display/redirect

# 2. консоль Jenkins-сборки (silence-mode: без вывода упавших тестов, но со сводкой ctest)
ssh ishurupov@supermarket-wm "curl -sk $SQ '$B/job/modules-initial/<N>/consoleText'" > /tmp/ci.log
grep -nE 'tests passed|tests failed out of|FAILED|error:|skipped due to earlier|script returned exit code' /tmp/ci.log

# 3. детали упавших тестов — из allure-отчёта (в консоли их НЕТ):
ssh ishurupov@supermarket-wm "curl -skL $SQ '$B/job/modules-initial/<N>/artifact/allure-report.zip' -o /tmp/a.zip"
#   распаковать, data/test-cases/<hash>.json: поля name/status/statusDetails.message и
#   testStage.steps[].name содержат реальную причину (напр. 'radioManager_x64 not built',
#   'only 0 of 3 instances discovered'). Имена .board-тестов берут из console-сводки.
# Артефакты сборки также есть по /artifact/... (напр. build/*/test-artifacts/<Test>/core*.log).
```

Чтобы отличить реальную регрессию от инфраструктурного сбоя — сравнить со СВЕЖЕЙ ЗЕЛЁНОЙ
сборкой той же джобы (`lastSuccessfulBuild` в `/job/<name>/api/json`), которая реально
ПРОГОНЯЛА тесты (не «stages skipped» из-за закрытого MR): если там те же тесты `Passed`,
а на ветке `Failed` — это регрессия ветки, а не окружения.

Перезапуск сборки: push в ветку, либо коммент в MR (`gl_mr.py note ... rebuilding`
для modules; `modules` в runtime-MR для второй стадии).

## Грабли

- **400 `line_code must be a valid line code`** — строка не входит в дифф-хунк, либо для
  context-строки не передан парный `old_line`. Скрипт это решает; при ручном curl —
  для неизменённой строки передавать И `old_line`, И `new_line`, для добавленной — только
  `new_line`, для удалённой — только `old_line`.
- Позиция обязана содержать `base_sha`/`start_sha`/`head_sha` из `diff_refs` **текущего**
  состояния MR. После нового пуша в ветку старые sha перестают работать — перечитать `info`.
- Комментарий к строке файла, которого нет в списке изменений MR, создать нельзя: якорить
  замечание к ближайшему изменённому месту и в тексте указать реальный `file:line`.
- `/changes` может обрезать большие диффы (`overflow: True`) — тогда только `git diff` в клоне.
- Крупные файлы GitLab отдаёт «свёрнутыми»: в `/changes` у них `collapsed: true` и пустой `diff`,
  из-за чего `post` пишет «строка вне дифф-хунков» и молча пропускает замечание. Лечится
  параметром `?access_raw_diffs=true` — он уже зашит в `get_changes()` в `gl_mr.py`. Если
  `--dry-run` внезапно даёт `SKIP` на файле, который точно менялся, — проверять именно это.
- Состояние ревьювера (`REQUESTED_CHANGES`/`REVIEWED`) в GitLab 18.x меняется **только**
  quick action в теле заметки: `PUT /merge_requests/:iid/reviewers/:user_id/state` и
  `/reviewer_state` отвечают 404, GraphQL-мутации для этого нет (в схеме только
  `mergeRequestReviewerRereview` и `mergeRequestSetReviewers`). Признак успеха — HTTP **202**
  с полем `summary` и **отсутствие** `id` в ответе: комментарий при этом не создаётся.
  Если в ответе есть `id`, команда не распозналась и в MR появился мусорный комментарий —
  удалить его (`DELETE /merge_requests/:iid/notes/:id`).
- `approve` и `request_changes` — разные вещи: `/approve` заполняет `approved_by` и снимает
  `not_approved`, но не годится, когда в MR оставлены замечания. Прочитать своё текущее
  состояние можно только через GraphQL (`reviewers { nodes { mergeRequestInteraction
  { reviewState } } }`) — в REST-ответе MR его нет.
- Ревью постится от имени владельца токена (nmotin), поэтому опечатка сразу видна всей команде:
  перечитать тексты замечаний перед `post`, а не после.
