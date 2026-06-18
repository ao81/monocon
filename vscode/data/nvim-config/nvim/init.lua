vim.treesitter.start = function() end 

-- leader を Space に設定する
-- ※ プラグインを読み込むより前に書くこと
vim.g.mapleader = " "
vim.g.maplocalleader = " "

-- Space 単体のデフォルト動作（カーソルを右に動かす）を無効化しておくと、
-- leader キーとして安定して動く
vim.keymap.set({ "n", "v" }, "<Space>", "<Nop>", { silent = true })

-- 行内の true/false を入れ替える
local function toggle_bool()
local line = vim.api.nvim_get_current_line()
local cur = vim.api.nvim_win_get_cursor(0)
local col = cur[2] -- 0 始まりのバイト位置

-- 行内の true / false を単語単位で集める
local matches = {}
for _, w in ipairs({ "true", "false" }) do
	local init = 1
	while true do
	local s, e = line:find("%f[%w]" .. w .. "%f[%W]", init)
	if not s then break end
	matches[#matches + 1] = { s = s, e = e, word = w }
	init = e + 1
	end
end
if #matches == 0 then return end
table.sort(matches, function(a, b) return a.s < b.s end)

-- カーソル位置以降を優先（無ければ最初のもの）
local chosen = matches[1]
for _, m in ipairs(matches) do
	if m.e >= col + 1 then
	chosen = m
	break
	end
end

local repl = chosen.word == "true" and "false" or "true"
local new = line:sub(1, chosen.s - 1) .. repl .. line:sub(chosen.e + 1)
vim.api.nvim_set_current_line(new)
vim.api.nvim_win_set_cursor(0, { cur[1], chosen.s - 1 })
end

vim.keymap.set("n", "<C-q>", toggle_bool, { desc = "toggle true/false" })

vim.api.nvim_create_autocmd("FileType", {
	pattern = "markdown",
	callback = function()
	pcall(vim.treesitter.stop)
	end,
})