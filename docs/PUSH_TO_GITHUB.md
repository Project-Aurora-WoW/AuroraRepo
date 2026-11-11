# Как запушить репозиторий на GitHub

Локальный репозиторий уже создан и содержит первый коммит:

```
C:\Users\Orel\Documents\MultiTool\HomeChats\Chat-3\AuroraRepo
```

```
64fd89d  Aurora 7.1.0: server module, SQL layer and client addon
38 файлов
```

Дальше — три способа опубликовать.

---

## Способ 1. Через сайт GitHub (самый простой)

1. Откройте https://github.com/new
2. **Repository name**: `aurora-wow` (или любое)
3. **Visibility**: Private / Public — на ваш выбор
4. **НЕ** ставьте галочки «Add README», «Add .gitignore», «Choose a license» — иначе будет конфликт истории
5. Нажмите **Create repository**
6. Скопируйте HTTPS-адрес, например:
   `https://github.com/maxorlov59rus/aurora-wow.git`
7. В PowerShell:

```powershell
cd "C:\Users\Orel\Documents\MultiTool\HomeChats\Chat-3\AuroraRepo"
git remote add origin https://github.com/maxorlov59rus/aurora-wow.git
git push -u origin main
```

При запросе пароля введите **Personal Access Token**, а не пароль от аккаунта.
Токен: GitHub → Settings → Developer settings → Personal access tokens → Tokens (classic) → Generate new token, права `repo`.

---

## Способ 2. Через GitHub CLI (если поставите)

```powershell
winget install --id GitHub.cli
gh auth login          # один раз, откроется браузер
cd "C:\Users\Orel\Documents\MultiTool\HomeChats\Chat-3\AuroraRepo"
gh repo create aurora-wow --private --source=. --remote=origin --push
```

`gh` сам создаст репозиторий и запушит.

---

## Способ 3. SSH-ключ (если уже настроен)

```powershell
cd "C:\Users\Orel\Documents\MultiTool\HomeChats\Chat-3\AuroraRepo"
git remote add origin git@github.com:maxorlov59rus/aurora-wow.git
git push -u origin main
```

---

## Права в GitHub

Модуль компилируется в один бинарник с AzerothCore, который распространяется под **GNU GPL v2**. Если делаете репозиторий публичным, лицензия должна быть GPL v2 — иначе будут вопросы о совместимости. Если репозиторий приватный, формальность можно отложить.

Файл `LICENSE` с текстом GPL v2 в репозиторий **пока не добавлен**. Добавьте его перед публикацией:

```powershell
# вариант через gh
gh api /licenses/gpl-2.0 --jq .body | Out-File -Encoding utf8 LICENSE
```

---

## Что уже в репозитории

```
README.md
BUILD.md
SQL_NOTES.md
.gitignore
src/Aurora/          серверный модуль (18 файлов)
sql/                 4 файла схемы и данных
addon/AuroraUI/      готовый аддон
addon/ElvUI_Aurora/  вариант на ElvUI
```

`.gitignore` отсекает бинарники, `.conf`, логи и бэкапы `*.bak`.

---

## Проверка после публикации

```powershell
git remote -v
git log --oneline -1
git ls-files | Measure-Object -Line
```

Должно быть 38 файлов и один коммит.

---

## Если надо переименовать ветку

Сейчас ветка `main`. Если GitHub создаст репозиторий с `master`:

```powershell
git branch -M main
git push -u origin main --force
```
