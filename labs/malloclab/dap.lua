local ok, dap = pcall(require, 'dap')
if not ok then
  print('dap is not ready.')
  return
end

vim.cmd [[
  augroup malloclab_dap
    autocmd!
    autocmd BufWritePost dap.lua :luafile %
  augroup end
]]

dap.configurations.c = {
  {
    name = 'test mdriver',
    type = 'cppdbg',
    request = 'launch',
    program = '${fileDirname}/mdriver',
    args = { '-c', './traces/short1-bal.rep', '-V', '-D' },
    cwd = '${fileDirname}',
    stopAtEntry = false
  }
}
