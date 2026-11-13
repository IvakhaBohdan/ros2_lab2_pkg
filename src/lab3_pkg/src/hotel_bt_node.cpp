#include <rclcpp/rclcpp.hpp>
#include <behaviortree_cpp/bt_factory.h>
#include <string>

using namespace BT;

struct RoomEnvironment
{
  bool has_pillows_and_sheets = false;
  int stains_remaining = 2; // Потребує 2 спроби очищення (ліміт: 3)
  int clean_attempts = 0;
  std::string season = "winter";
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("hotel_bt_node");
  auto logger = node->get_logger();

  node->declare_parameter<std::string>("season", "winter");

  RoomEnvironment env;
  env.season = node->get_parameter("season").as_string();

  BehaviorTreeFactory factory;

  factory.registerSimpleCondition("CheckItemsPresent", [&](TreeNode &) {
    RCLCPP_INFO(logger, "[BT: CheckItemsPresent] Перевірка наявності подушок та простирадл...");
    if (env.has_pillows_and_sheets) {
      RCLCPP_INFO(logger, "  -> Усе на місці (SUCCESS)");
      return NodeStatus::SUCCESS;
    }
    RCLCPP_WARN(logger, "  -> Подушки/простирадла відсутні (FAILURE)");
    return NodeStatus::FAILURE;
  });

  factory.registerSimpleAction("RestockItems", [&](TreeNode &) {
    RCLCPP_INFO(logger, "[BT: RestockItems] Доставляємо та розкладаємо подушки й простирадла...");
    env.has_pillows_and_sheets = true;
    return NodeStatus::SUCCESS;
  });

  factory.registerSimpleCondition("IsBedClean", [&](TreeNode &) {
    RCLCPP_INFO(logger, "[BT: IsBedClean] Перевірка ліжка на наявність плям...");
    if (env.stains_remaining <= 0) {
      RCLCPP_INFO(logger, "  -> Плям немає, ліжко чисте (SUCCESS)");
      return NodeStatus::SUCCESS;
    }
    RCLCPP_WARN(logger, "  -> Виявлено плями! Залишилось: %d (FAILURE)", env.stains_remaining);
    return NodeStatus::FAILURE;
  });

  factory.registerSimpleAction("CleanStainAttempt", [&](TreeNode &) {
    env.clean_attempts++;
    RCLCPP_INFO(logger, "[BT: CleanStainAttempt] Спроба очищення #%d з 3...", env.clean_attempts);
    env.stains_remaining--;
    if (env.stains_remaining <= 0) {
      RCLCPP_INFO(logger, "  -> Усі плями успішно видалено! (SUCCESS)");
      return NodeStatus::SUCCESS;
    }
    RCLCPP_WARN(logger, "  -> Пляма ще залишилась (FAILURE -> повтор через Retry)");
    return NodeStatus::FAILURE;
  });

  factory.registerSimpleCondition("IsWinter", [&](TreeNode &) {
    RCLCPP_INFO(logger, "[BT: IsWinter] Перевірка сезону (поточний: '%s')...", env.season.c_str());
    return (env.season == "winter") ? NodeStatus::SUCCESS : NodeStatus::FAILURE;
  });

  factory.registerSimpleAction("ApplyWinterBedding", [&](TreeNode &) {
    RCLCPP_INFO(logger, "[BT: ApplyWinterBedding] Заміна білизни та застилання теплої ЗИМОВОЇ ковдри (SUCCESS)");
    return NodeStatus::SUCCESS;
  });

  factory.registerSimpleAction("ApplySummerBedding", [&](TreeNode &) {
    RCLCPP_INFO(logger, "[BT: ApplySummerBedding] Заміна білизни та застилання легкої ЛІТНЬОЇ ковдри (SUCCESS)");
    return NodeStatus::SUCCESS;
  });

  static const char * xml_text = R"(
  <root main_tree_to_execute="HotelCleaningTree">
    <BehaviorTree ID="HotelCleaningTree">
      <Sequence name="MainCleaningSequence">

        <Fallback name="EnsureItemsPresent">
          <CheckItemsPresent/>
          <RestockItems/>
        </Fallback>

        <Fallback name="EnsureBedClean">
          <IsBedClean/>
          <RetryUntilSuccessful num_attempts="3">
            <CleanStainAttempt/>
          </RetryUntilSuccessful>
        </Fallback>

        <Fallback name="SeasonalBedding">
          <Sequence name="WinterBranch">
            <IsWinter/>
            <ApplyWinterBedding/>
          </Sequence>
          <ApplySummerBedding/>
        </Fallback>

      </Sequence>
    </BehaviorTree>
  </root>
  )";

  RCLCPP_INFO(logger, "=== Запуск BehaviorTree.CPP (сезон: %s) ===", env.season.c_str());
  auto tree = factory.createTreeFromText(xml_text);
  NodeStatus status = tree.tickOnce();
  RCLCPP_INFO(logger, "=== Результат виконання дерева: %s ===", toStr(status).c_str());

  rclcpp::shutdown();
  return 0;
}
