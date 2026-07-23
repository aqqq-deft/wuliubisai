# 用普通 ChatGPT 跟踪物流比赛项目的操作指南

> 适用仓库：`aqqq-deft/wuliubisai`
> 默认分支：`main`
> 目标：让普通 ChatGPT 读取 GitHub 中的最新项目状态并指导操作，尽量减少 Codex 用量

## 先看结论

你可以把私有 GitHub 仓库连接到普通 ChatGPT，然后在普通对话中让它读取代码、文档和 `PROJECT_STATUS.md`，回答项目问题。

需要注意：

- 普通 ChatGPT 不会一直在后台主动监控项目；每次有新进展后，需要你主动让它重新读取仓库。
- ChatGPT 只能看到已经推送到 GitHub 的内容，看不到电脑上尚未提交或尚未推送的文件。
- GitHub 应用主要用于读取、分析和搜索，不能代替你修改并推送代码。
- 在普通 ChatGPT 对话中读取 GitHub，不会因为连接了仓库就自动变成 Codex 任务；但仍会使用普通 ChatGPT 的消息或模型额度。
- 如果你切换到 Codex、Agent、ChatGPT Work 等智能体功能，可能会使用相应的智能体用量或共享额度。

本项目最重要的状态入口是：

```text
PROJECT_STATUS.md
```

以后不要让 ChatGPT 每次从几百个文件中猜进度，要先让它读取这个文件。

## 一、第一次连接 GitHub

建议优先使用 ChatGPT 网页版：

```text
https://chatgpt.com
```

### 第 1 步：打开 GitHub 应用

由于 ChatGPT 界面会更新，你可能看到以下任意一种入口。

当前新版入口：

1. 打开 ChatGPT。
2. 打开插件目录（Plugins Directory）。
3. 搜索 `GitHub`。
4. 打开包含 GitHub 应用的插件详情。
5. 点击 `Connect`。

如果仍是旧版界面：

1. 点击左下角头像。
2. 进入“设置（Settings）”。
3. 进入“应用（Apps）”。
4. 找到 `GitHub`。
5. 点击“连接（Connect）”。

### 第 2 步：登录并授权 GitHub

1. 浏览器会跳转到 GitHub。
2. 登录账号 `aqqq-deft`。
3. 找到仓库访问范围设置。
4. 优先选择“仅选择的仓库（Only select repositories）”。
5. 只勾选：

   ```text
   wuliubisai
   ```

6. 确认安装或授权。

不要为了省事授权所有私有仓库。

### 第 3 步：确认私有仓库权限

返回 ChatGPT 后：

1. 打开“设置 → 应用 → GitHub”。
2. 点击齿轮、`Choose repositories` 或 `Configure repositories on GitHub`。
3. 确认 `aqqq-deft/wuliubisai` 在允许列表中。
4. 如果仓库是刚创建或刚改为私有，等待约 5～10 分钟。

### 第 4 步：仓库仍然找不到时

在 GitHub 网站顶部搜索框中输入：

```text
repo:aqqq-deft/wuliubisai import
```

执行搜索后等待约 5～10 分钟，再回到 ChatGPT 重试。

## 二、确认你使用的是普通 ChatGPT

为了减少 Codex 用量，请这样检查：

1. 使用普通 ChatGPT 对话，不打开 Codex。
2. 不选择 Agent 模式。
3. 不选择 ChatGPT Work。
4. 普通项目问答通常不需要 Deep Research。
5. 只从输入框旁边的“+ / 工具”选择 GitHub，或在提示词中使用 `@GitHub`。

如果 GitHub 只出现在 Agent 或 Deep Research 中，而普通聊天里没有，说明当前套餐或界面暂不支持普通聊天直接调用 GitHub。此时不要为了省 Codex 额度而强行使用 Agent，可以采用本指南后面的“手动上传文件方案”。

## 三、创建专用的普通 ChatGPT 对话

建议为这个比赛项目单独新建一个普通对话，例如命名为：

```text
物流比赛电控指导
```

在输入框的“+ / 工具”中选中 GitHub，然后发送下面这段提示词。

### 首次初始化提示词

```text
请连接并读取 GitHub 私有仓库 aqqq-deft/wuliubisai 的 main 分支。

请先完整阅读根目录的 PROJECT_STATUS.md，再按需阅读交接2.md、电控工作区/README.md、电控工作区/docs/ 和与当前问题直接相关的源码。

这是一个物流比赛 STM32 电控项目，我是初学者。请严格区分：
1. 已实物验证；
2. 代码已实现但未完整验证；
3. 计划中。

不要因为代码存在就声称硬件已经测试成功。旧文档和当前源码不一致时，要明确指出差异：当前源码用于判断代码状态，最新测试记录用于判断验证状态。

回答时先告诉我当前唯一最重要的下一步，再一次只给我一个可以照做的操作。每个操作都要包含：使用的软件、真实文件路径、点击位置或命令、预期现象、如何判断成功，以及安全注意事项。

涉及接电、烧录、电机、驱动器、修改固件或删除文件时，先提醒风险并等待我确认。不要让我在 RTX 和完整任务未验证前连接 12 V 电池或启动电机。

请先回复：
1. PROJECT_STATUS.md 的最后更新日期；
2. 当前一句话状态；
3. 当前唯一下一步；
4. 你还缺少什么信息。
```

如果 ChatGPT 能正确说出状态文件中的日期和“一句话状态”，说明它已经读到仓库。

## 四、以后每次怎么使用

### 情况 A：只是询问当前下一步

```text
请通过 GitHub 重新读取 aqqq-deft/wuliubisai 的 main 分支和 PROJECT_STATUS.md。不要使用上一次回答的旧状态。告诉我当前唯一最重要的下一步，并一次只给我一个操作。
```

### 情况 B：你刚刚推送了新代码或新文档

```text
我刚刚更新了 GitHub 仓库 aqqq-deft/wuliubisai 的 main 分支。请重新读取 PROJECT_STATUS.md 和与本次修改相关的源码，不要沿用缓存。请列出：本次确认的新进展、仍未验证的事项、当前阻塞和下一步唯一动作。
```

### 情况 C：遇到报错

把完整报错文本粘贴到普通 ChatGPT，或者上传清晰截图，然后发送：

```text
请先读取 GitHub 仓库 aqqq-deft/wuliubisai 的 PROJECT_STATUS.md 和相关源码，再分析下面的报错。

请先判断这是环境、配置、编译、烧录、RTX、串口还是硬件问题。不要一次给多个方向，只给最可能原因对应的第一步检查，并说明预期结果。

完整报错如下：
[在这里粘贴完整报错]
```

### 情况 D：完成了一项实物测试

```text
下面是我刚完成的真实测试结果。请根据这些证据生成一段可以粘贴到 PROJECT_STATUS.md 的最小更新内容。

要求：
1. 只把有真实证据的事项写入“已实物验证”；
2. 不要把推测写成结论；
3. 保留尚未验证项；
4. 给出新的“当前卡点”和“下一步唯一动作”；
5. 不要修改无关章节。

测试日期：
测试步骤：
使用的固件或提交：
实际观察：
完整日志或报错：
```

ChatGPT 输出后，你需要自己检查内容，再按“第六部分”更新 GitHub。

### 情况 E：询问某段代码

```text
请通过 GitHub 读取 aqqq-deft/wuliubisai 中的以下文件：
[填写仓库内路径]

请结合 PROJECT_STATUS.md 解释这段代码在当前项目里的作用。先用大白话说明，再指出关键函数和执行顺序。不要修改代码，也不要假设未完成的硬件测试已经通过。
```

## 五、最省 Codex 额度的日常流程

建议固定使用下面的循环：

1. 普通 ChatGPT 读取 `PROJECT_STATUS.md`，告诉你当前一步。
2. 你按照指导完成一次操作。
3. 把真实结果、日志或截图发回普通 ChatGPT。
4. 普通 ChatGPT 根据证据生成状态更新片段。
5. 你在 GitHub 网页上手动更新 `PROJECT_STATUS.md`。
6. 再让普通 ChatGPT 重新读取 `main`。

只在以下情况使用 Codex：

- 需要直接修改多个代码文件。
- 需要在本地运行命令、编译或检查仓库。
- 需要自动提交、推送或创建 PR。
- 普通 ChatGPT 已经定位问题，但你不会安全地完成代码修改。

以下工作优先使用普通 ChatGPT：

- 解释报错。
- 阅读已经推送的代码。
- 讲解操作步骤。
- 根据日志判断下一步。
- 整理实验结果。
- 生成可以手动粘贴的状态更新片段。

## 六、不使用 Codex，手动更新项目进度

### 方法 1：直接在 GitHub 网页编辑状态文件

状态文档的小改动可以直接在 GitHub 网页完成：

1. 打开：

   ```text
   https://github.com/aqqq-deft/wuliubisai
   ```

2. 确认左上角分支是 `main`。
3. 点击 `PROJECT_STATUS.md`。
4. 点击右上角铅笔图标 `Edit this file`。
5. 根据普通 ChatGPT 生成的内容做最小修改。
6. 点击 `Commit changes...`。
7. 提交说明填写：

   ```text
   更新项目进度
   ```

8. 如果页面允许选择，状态文档的小更新可以直接提交到 `main`。
9. 提交完成后等待几分钟，再让普通 ChatGPT 重新读取仓库。

不要用 GitHub 网页随意编辑厂家 GBK 编码的 C/H 源码。网页编辑只建议用于 `PROJECT_STATUS.md` 等 UTF-8 Markdown 文档。

### 方法 2：普通聊天里手动上传文件

如果你的套餐在普通聊天里看不到 GitHub 应用，可以使用这个备用方法：

1. 从 GitHub 下载或打开 `PROJECT_STATUS.md`。
2. 在普通 ChatGPT 对话中上传这个文件。
3. 遇到具体代码问题时，再补充上传相关的少量源码文件。
4. 明确告诉 ChatGPT：只根据本次上传文件判断，不要假装能访问整个仓库。

可复制提示词：

```text
我当前不能在普通聊天中使用 GitHub 应用。请只根据我本次上传的 PROJECT_STATUS.md 和相关源码回答，不要假装能访问 GitHub，也不要使用旧文件状态。先告诉我当前唯一下一步，一次只给一个操作。
```

这种方式同样不需要 Codex，但每次项目更新后需要重新上传最新文件。

## 七、如何判断 ChatGPT 读到的是最新版本

每次重要更新后，让 ChatGPT 回答以下内容：

```text
请重新读取 main 分支的 PROJECT_STATUS.md，并原样告诉我：
1. “最后更新”日期；
2. “一句话状态”内容；
3. “当前卡点”标题下的核心结论；
4. “下一步操作”中的第一步。
```

然后与 GitHub 网页上的文件核对。

如果不一致：

1. 确认修改已经提交到 `main`，不是只在其他分支或未合并 PR 中。
2. 等待 5～10 分钟。
3. 在本次消息中重新选择 GitHub 工具。
4. 明确要求“重新读取，不使用上一次回答的旧状态”。
5. 仍不更新时，新建一个普通对话再次测试。

## 八、常见问题排查

### 1. ChatGPT 找不到仓库

依次检查：

1. GitHub 登录账号是否为 `aqqq-deft`。
2. ChatGPT 的 GitHub 应用是否已连接。
3. GitHub 应用是否被授权访问 `wuliubisai`。
4. 仓库是否仍为私有且当前账号有权限。
5. 是否等待了至少 5～10 分钟。
6. 是否执行过：

   ```text
   repo:aqqq-deft/wuliubisai import
   ```

### 2. 普通聊天里没有 GitHub

这可能是套餐、工作区权限、地区、客户端版本或当前产品界面的限制。

处理顺序：

1. 使用最新版 ChatGPT 网页版。
2. 检查插件目录和“设置 → 应用”。
3. 确认 GitHub 显示为已连接。
4. 如果只在 Agent 或 Deep Research 中可用，为节省额度，改用“手动上传文件方案”。

### 3. ChatGPT 回答得像在猜

在问题开头加上：

```text
必须先通过 GitHub 重新读取 PROJECT_STATUS.md 和相关源码。无法读取时请直接说无法读取，不要根据记忆猜测。
```

### 4. ChatGPT 看不到电脑上的最新修改

原因通常是文件只改在本地，还没有推送到 GitHub。普通 ChatGPT 的 GitHub 应用无法读取未推送的本地文件。

处理方法：

- 先手动提交并推送；或
- 直接把未推送的文件上传到当前普通 ChatGPT 对话。

### 5. ChatGPT 能否帮我推送代码

GitHub 应用本身主要是只读分析。需要自动修改、提交和推送代码时，才使用 Codex或你熟悉的本地 Git 工具。

## 九、隐私和安全设置

本仓库是私有仓库，建议保持以下设置：

1. GitHub 应用只授权 `wuliubisai`，不要授权全部仓库。
2. 不要把 Token、密码、Cookie、验证码、私钥或 `.env` 上传到仓库。
3. 在 ChatGPT“设置 → 数据控制”中检查“为所有人改进模型（Improve the model for everyone）”。
4. 如果使用个人订阅且不希望对话内容用于改进模型，可以关闭该设置。
5. 如果断开 GitHub 应用，未来同步会停止，但过去已经使用仓库数据的对话不会自动删除；不再需要时应同时删除相关对话和已保存记忆。

## 十、额度使用原则

为了尽量少用 Codex：

- 普通问答、代码解释、日志分析和步骤指导：使用普通 ChatGPT。
- GitHub 应用读取仓库：优先放在普通 ChatGPT 对话中。
- 自动改代码、运行本地命令、提交和推送：只在确实需要时使用 Codex。
- 不要把普通问答切换成 Agent、ChatGPT Work 或 Codex 任务。
- 可以在 Codex 的 `Settings → Usage` 中查看使用情况，前后对比确认自己的套餐如何计量。

具体额度规则可能随套餐和产品更新而变化，以你账号中的 Usage 页面为准。

## 十一、官方参考

- [Connecting GitHub to ChatGPT](https://help.openai.com/en/articles/11145903-connecting-github-to-chatgpt-deep-research-to-chatgpt-deep-research)
- [ChatGPT apps with sync](https://help.openai.com/en/articles/10847137-chatgpt-synced-connectors)
- [Apps in ChatGPT](https://help.openai.com/en/articles/11487775-connectors-in-chatgpt.otf)
- [Using Credits for Flexible Usage in ChatGPT](https://help.openai.com/en/articles/12642688-using-credits-for-flexible-usage-in-chatgpt-plus-pro)
