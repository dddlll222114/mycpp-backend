const votes = [];

module.exports = async (req, res) => {
  // 开启跨域，让前端能正常访问
  res.setHeader('Access-Control-Allow-Origin', '*');
  res.setHeader('Access-Control-Allow-Methods', 'GET, POST, OPTIONS');
  res.setHeader('Access-Control-Allow-Headers', 'Content-Type');

  // 处理预检请求
  if (req.method === 'OPTIONS') {
    return res.status(200).end();
  }

  // 接口1：获取投票列表
  if (req.method === 'GET' && req.url === '/api/index/list') {
    return res.json(votes);
  }

  // 接口2：创建投票
  if (req.method === 'POST' && req.url === '/api/index/create') {
    const { title, options } = req.body;
    const newVote = {
      id: Date.now().toString(),
      topic: title,
      options: options,
      counts: options.map(() => 0)
    };
    votes.push(newVote);
    return res.json({ success: true, id: newVote.id });
  }

  // 接口3：提交投票
  if (req.method === 'POST' && req.url === '/api/index/vote') {
    const { voteId, optIndex } = req.body;
    const vote = votes.find(v => v.id === voteId);
    if (vote) {
      vote.counts[optIndex]++;
      return res.json({ success: true });
    }
    return res.status(404).json({ error: '投票不存在' });
  }

  // 其他请求返回404
  res.status(404).json({ error: 'Not found' });
};
