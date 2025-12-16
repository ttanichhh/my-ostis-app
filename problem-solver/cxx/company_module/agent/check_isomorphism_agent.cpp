#include "check_isomorphism_agent.hpp"
#include "../keynodes/org_keynodes.hpp"
#include "utils.hpp"

#include <sc-memory/sc_memory.hpp>
#include <algorithm>
#include <string>
#include <unordered_map>
#include <vector>
#include <queue>
#include <set>

using namespace std;

//Этот агент запускается, когда инициируется действие
//action_check_graph_isomorphism
ScAddr CheckIsomorphismAgent::GetActionClass() const
{
  return OrgKeynodes::action_check_graph_isomorphism;
}

//собирает в вектор все узлы, связанные с исходным node отношением nrel
static void collect_children(ScMemoryContext & ctx,
                             const ScAddr & node,
                             const ScAddr & nrel,
                             vector<ScAddr> & out_children)
{
  //Это 5-элементный итератор SC-памяти. 
  //Он ищет конструкции вида: узел->первая дуга (ребро) ->второй узел ->вторая дуга->узел-отношение
  ScIterator5Ptr it = ctx.CreateIterator5(
    node,
    ScType::ConstCommonArc,
    ScType::Unknown,
    ScType::ConstPermPosArc,
    nrel
  );

  //перебирает все найденные связи и фиксирует все потомков узла node
  //В результате out_children содержит всех потомков узла node,
  //связанных через заданное nrel
  while (it->Next())
  {
    ScAddr child = it->Get(2);
    if (child.IsValid())
      out_children.push_back(child);
  }
}

// строим внутреннее представление графа оргструктуры, начиная с корня
static void build_graph_from_root(ScMemoryContext & ctx,
                                  const ScAddr & root,
                                  const ScAddr & rel_high,
                                  const ScAddr & rel_work,
                                  unordered_map<size_t, vector<size_t>> & adj, //список смежности
                                  unordered_set<size_t> & nodes_set) //собирает все уникальные вершины
{
  queue<ScAddr> q;
  unordered_set<size_t> visited;
  if (!root.IsValid()) return;
  q.push(root);
  visited.insert(root.Hash());
  nodes_set.insert(root.Hash());

  while (!q.empty())
  {
    ScAddr cur = q.front(); q.pop();
    size_t curh = cur.Hash();
    vector<ScAddr> children;
    // сохраняем оба отношения
    collect_children(ctx, cur, rel_high, children);
    collect_children(ctx, cur, rel_work, children);

    for (ScAddr & ch : children)
    {
      if (!ch.IsValid()) continue;
      size_t chh = ch.Hash();
      adj[curh].push_back(chh);
      nodes_set.insert(chh);
      if (visited.find(chh) == visited.end())
      {
        visited.insert(chh);
        q.push(ch);
      }
    }
  }
}

//превращает поддерево с корнем v в строку, которая:
//одинакова для всех изоморфных поддеревьев
//разная, если структура поддеревьев разная
static string canonical_form(size_t v,
                             unordered_map<size_t, vector<size_t>> & adj,
                             unordered_map<size_t, string> & memo) //текущая вершина, список смежности и кэш
{
  auto itm = memo.find(v);
  if (itm != memo.end()) return itm->second;

  vector<string> child_forms;
  auto it = adj.find(v);
  if (it != adj.end())
  {
    for (size_t ch : it->second)
      child_forms.push_back(canonical_form(ch, adj, memo));
  }
  sort(child_forms.begin(), child_forms.end());
  string res = "(";
  for (auto & s : child_forms) res += s;
  res += ")";
  memo[v] = res;
  return res;
}

// смотрим максимальную глубину и ширину для объснения различия структур, если они не изоморфны
static void compute_depth_and_degree(size_t root,
                                     unordered_map<size_t, vector<size_t>> & adj, //список подчиненных
                                     int & max_depth,
                                     int & max_deg,
                                     unordered_map<size_t, int> & depth_cache) //кэш глубины вершин
{
  //обход в глубину, для каждой вершины считаем глубину поддерева и кол-ва детей
  function<int(size_t)> dfs = [&](size_t v)->int {
    if (depth_cache.find(v) != depth_cache.end()) return depth_cache[v];
    int best = 0; //максимальная глубина среди детей
    auto it = adj.find(v);
    int deg = 0; //количество прямых подчинённых
    if (it != adj.end())
    {
      deg = (int)it->second.size();
      for (size_t ch : it->second) //рекурсивный обход детей
      {
        int d = dfs(ch);
        if (d > best) best = d;
      }
    }
    max_deg = max(max_deg, deg);
    depth_cache[v] = 1 + best;
    max_depth = max(max_depth, depth_cache[v]);
    return depth_cache[v];
  };

  dfs(root); //Запускаем обход со всей оргструктуры
}

// строит взаимно-однозначное соответствие вершин между двумя уже признанными изоморфными деревьями
// соотнощение должностей
static void build_mapping(size_t a, size_t b, //вершины графов
                          unordered_map<size_t, vector<size_t>> & adjA, //списки подчиненных
                          unordered_map<size_t, vector<size_t>> & adjB,
                          unordered_map<size_t, string> & memoA, //формы вершин
                          unordered_map<size_t, string> & memoB,
                          vector<pair<size_t,size_t>> & out_pairs) // получившийся список соответствий
{
  //получаем детей
  out_pairs.emplace_back(a,b);
  vector<size_t> ca = adjA.count(a) ? adjA[a] : vector<size_t>{};
  vector<size_t> cb = adjB.count(b) ? adjB[b] : vector<size_t>{};

  //группируем детей
  unordered_map<string, vector<size_t>> ga, gb;
  for (size_t ch : ca) ga[memoA[ch]].push_back(ch);
  for (size_t ch : cb) gb[memoB[ch]].push_back(ch);

  for (auto & p : ga) //идем по 1 группе детей и ищем такую же во 2
  {
    const string & key = p.first; //key — форма поддерева
    auto & vecA = p.second;
    auto itb = gb.find(key);
    if (itb == gb.end()) continue; 
    auto & vecB = itb->second;
    size_t n = min(vecA.size(), vecB.size());
    for (size_t i = 0; i < n; ++i)
      build_mapping(vecA[i], vecB[i], adjA, adjB, memoA, memoB, out_pairs);
  }
}

ScResult CheckIsomorphismAgent::DoProgram(ScAction & action)
{
  auto const & [argsAddr] = action.GetArguments<1>();

  if (!m_context.IsElement(argsAddr))
    return action.FinishWithError();

  ScAddr const root1 = resolve_arg(m_context, argsAddr, OrgKeynodes::rrel_1);
  ScAddr const root2 = resolve_arg(m_context, argsAddr, OrgKeynodes::rrel_2);

  if (!root1.IsValid() || !root2.IsValid())
    return action.FinishWithError();

  unordered_map<size_t, vector<size_t>> adjA, adjB;
  unordered_set<size_t> nodesA, nodesB;
  build_graph_from_root(m_context, root1, OrgKeynodes::nrel_work_high_connection, OrgKeynodes::nrel_work_connection, adjA, nodesA);
  build_graph_from_root(m_context, root2, OrgKeynodes::nrel_work_high_connection, OrgKeynodes::nrel_work_connection, adjB, nodesB);

  unordered_map<size_t, string> memoA, memoB;
  string canA = canonical_form(root1.Hash(), adjA, memoA);
  string canB = canonical_form(root2.Hash(), adjB, memoB);

  ScStructure result = m_context.GenerateStructure();

  ScAddr isomorphicFlag = m_context.GenerateNode(ScType::ConstNode);
  if (canA == canB)
  {

    vector<pair<size_t,size_t>> pairs;
    build_mapping(root1.Hash(), root2.Hash(), adjA, adjB, memoA, memoB, pairs);

  }

  int countA = (int)nodesA.size();
  int countB = (int)nodesB.size();

  int max_depthA = 0, max_degA = 0;
  int max_depthB = 0, max_degB = 0;
  unordered_map<size_t,int> depth_cacheA, depth_cacheB;
  compute_depth_and_degree(root1.Hash(), adjA, max_depthA, max_degA, depth_cacheA);
  compute_depth_and_degree(root2.Hash(), adjB, max_depthB, max_degB, depth_cacheB);

  ScAddr nodeCountA = m_context.GenerateNode(ScType::ConstNode);
  ScAddr nodeCountB = m_context.GenerateNode(ScType::ConstNode);
  ScAddr depthA = m_context.GenerateNode(ScType::ConstNode);
  ScAddr depthB = m_context.GenerateNode(ScType::ConstNode);
  ScAddr degA = m_context.GenerateNode(ScType::ConstNode);
  ScAddr degB = m_context.GenerateNode(ScType::ConstNode);

  result << nodeCountA << nodeCountB << depthA << depthB << degA << degB << isomorphicFlag;

  m_context.GenerateConnector(ScType::ConstCommonArc, root1, nodeCountA);
  m_context.GenerateConnector(ScType::ConstCommonArc, root2, nodeCountB);
  m_context.GenerateConnector(ScType::ConstCommonArc, root1, depthA);
  m_context.GenerateConnector(ScType::ConstCommonArc, root2, depthB);
  m_context.GenerateConnector(ScType::ConstCommonArc, root1, degA);
  m_context.GenerateConnector(ScType::ConstCommonArc, root2, degB);

  if (canA == canB)
  {
    ScAddr markerTrue = m_context.GenerateNode(ScType::ConstNode);
    m_context.GenerateConnector(ScType::ConstPermPosArc, isomorphicFlag, markerTrue);
    result << markerTrue;
  }
  else
  {
    ScAddr markerFalse = m_context.GenerateNode(ScType::ConstNode);
    m_context.GenerateConnector(ScType::ConstPermPosArc, isomorphicFlag, markerFalse);
  }

  action.SetResult(result);
  return action.FinishSuccessfully();
}
