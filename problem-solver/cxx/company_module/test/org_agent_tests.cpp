#include <sc-memory/test/sc_test.hpp>
#include <sc-memory/sc_memory.hpp>

#include "../agent/check_isomorphism_agent.hpp"
#include "../keynodes/org_keynodes.hpp"

using AgentTest = ScMemoryTest;

void link_relation(ScMemoryContext * ctx, ScAddr src, ScAddr trg, ScAddr rel)
{
    ScAddr arc = ctx->GenerateConnector(ScType::ConstCommonArc, src, trg);
    ctx->GenerateConnector(ScType::ConstPermPosArc, rel, arc);
}

void create_sample_tree1(ScMemoryContext * ctx, ScAddr & root)
{
    root = ctx->GenerateNode(ScType::ConstNode);
    ScAddr c1 = ctx->GenerateNode(ScType::ConstNode);
    ScAddr c2 = ctx->GenerateNode(ScType::ConstNode);
    ScAddr g1 = ctx->GenerateNode(ScType::ConstNode);

    link_relation(ctx, root, c1, OrgKeynodes::nrel_work_high_connection);
    link_relation(ctx, root, c2, OrgKeynodes::nrel_work_high_connection);
    link_relation(ctx, c1, g1, OrgKeynodes::nrel_work_connection);
}

void create_sample_tree2(ScMemoryContext * ctx, ScAddr & root)
{
    root = ctx->GenerateNode(ScType::ConstNode);
    ScAddr a = ctx->GenerateNode(ScType::ConstNode);
    ScAddr b = ctx->GenerateNode(ScType::ConstNode);
    ScAddr c = ctx->GenerateNode(ScType::ConstNode);

    link_relation(ctx, root, a, OrgKeynodes::nrel_work_high_connection);
    link_relation(ctx, root, b, OrgKeynodes::nrel_work_high_connection);
    link_relation(ctx, a, c, OrgKeynodes::nrel_work_connection);
}

TEST_F(AgentTest, IsomorphicTrees)
{
    m_ctx->SubscribeAgent<CheckIsomorphismAgent>();

    ScAddr r1, r2;
    create_sample_tree1(m_ctx.get(), r1);
    create_sample_tree2(m_ctx.get(), r2);

    ScAddr args = m_ctx->GenerateNode(ScType::ConstNode);
    ScAddr a1 = m_ctx->GenerateConnector(ScType::ConstPermPosArc, args, r1);
    m_ctx->GenerateConnector(ScType::ConstPermPosArc, OrgKeynodes::rrel_1, a1);
    ScAddr a2 = m_ctx->GenerateConnector(ScType::ConstPermPosArc, args, r2);
    m_ctx->GenerateConnector(ScType::ConstPermPosArc, OrgKeynodes::rrel_2, a2);

    ScAction action = m_ctx->GenerateAction(OrgKeynodes::action_check_graph_isomorphism);
    action.SetArguments(args);

    EXPECT_TRUE(action.InitiateAndWait());
    EXPECT_TRUE(action.IsFinishedSuccessfully());

    ScAddr res = action.GetResult();
    EXPECT_TRUE(m_ctx->IsElement(res));
}

TEST_F(AgentTest, DifferentTrees)
{
    m_ctx->SubscribeAgent<CheckIsomorphismAgent>();

    ScAddr r1 = m_ctx->GenerateNode(ScType::ConstNode);
    ScAddr a = m_ctx->GenerateNode(ScType::ConstNode);
    ScAddr b = m_ctx->GenerateNode(ScType::ConstNode);
    link_relation(m_ctx.get(), r1, a, OrgKeynodes::nrel_work_high_connection);
    link_relation(m_ctx.get(), a, b, OrgKeynodes::nrel_work_high_connection);

    ScAddr r2 = m_ctx->GenerateNode(ScType::ConstNode);
    ScAddr c = m_ctx->GenerateNode(ScType::ConstNode);
    link_relation(m_ctx.get(), r2, c, OrgKeynodes::nrel_work_high_connection);

    ScAddr args = m_ctx->GenerateNode(ScType::ConstNode);
    ScAddr p1 = m_ctx->GenerateConnector(ScType::ConstPermPosArc, args, r1);
    m_ctx->GenerateConnector(ScType::ConstPermPosArc, OrgKeynodes::rrel_1, p1);
    ScAddr p2 = m_ctx->GenerateConnector(ScType::ConstPermPosArc, args, r2);
    m_ctx->GenerateConnector(ScType::ConstPermPosArc, OrgKeynodes::rrel_2, p2);

    ScAction action = m_ctx->GenerateAction(OrgKeynodes::action_check_graph_isomorphism);
    action.SetArguments(args);

    EXPECT_TRUE(action.InitiateAndWait());
    EXPECT_TRUE(action.IsFinishedSuccessfully());
    ScAddr res = action.GetResult();
    EXPECT_TRUE(m_ctx->IsElement(res));
}
