#include <rclcpp/rclcpp.hpp>
#include <smacc2/smacc.hpp>
#include <boost/mpl/list.hpp>
#include <string>

namespace mpl = boost::mpl;
using namespace smacc2;

struct EvItemsOk : sc::event<EvItemsOk> {};
struct EvItemsMissing : sc::event<EvItemsMissing> {};
struct EvItemsRestocked : sc::event<EvItemsRestocked> {};
struct EvStainsFound : sc::event<EvStainsFound> {};
struct EvNoStains : sc::event<EvNoStains> {};
struct EvCleanRetry : sc::event<EvCleanRetry> {};
struct EvCleanFailed : sc::event<EvCleanFailed> {};
struct EvWinter : sc::event<EvWinter> {};
struct EvSummer : sc::event<EvSummer> {};
struct EvBeddingDone : sc::event<EvBeddingDone> {};

struct StCheckItems;
struct StRestockItems;
struct StCheckStains;
struct StCleanStains;
struct StSelectSeason;
struct StWinterBedding;
struct StSummerBedding;
struct StDone;

struct SmHotelCleaner : public smacc2::SmaccStateMachineBase<SmHotelCleaner, StCheckItems>
{
  using SmaccStateMachineBase::SmaccStateMachineBase;

  bool has_pillows_and_sheets = false;
  int stains_remaining = 2;
  int clean_attempts = 0;
  std::string season = "winter";

  void onInitialize() override
  {
    this->getNode()->declare_parameter<std::string>("season", "winter");
    season = this->getNode()->get_parameter("season").as_string();
    RCLCPP_INFO(getLogger(), "[SMACC2] Ініціалізація автомата прибирання (сезон: %s)...", season.c_str());
  }
};

struct StCheckItems : smacc2::SmaccState<StCheckItems, SmHotelCleaner>
{
  using SmaccState::SmaccState;
  typedef mpl::list<
    smacc2::Transition<EvItemsOk, StCheckStains>,
    smacc2::Transition<EvItemsMissing, StRestockItems>
  > reactions;

  void onEntry()
  {
    auto & sm = this->context<SmHotelCleaner>();
    RCLCPP_INFO(getLogger(), "[Стан: StCheckItems] Перевірка подушок та простирадл...");
    if (sm.has_pillows_and_sheets) {
      RCLCPP_INFO(getLogger(), "  -> Подушки та простирадла в наявності.");
      this->postEvent<EvItemsOk>();
    } else {
      RCLCPP_WARN(getLogger(), "  -> Відсутні подушки/простирадла! Потрібне поповнення.");
      this->postEvent<EvItemsMissing>();
    }
  }
};

struct StRestockItems : smacc2::SmaccState<StRestockItems, SmHotelCleaner>
{
  using SmaccState::SmaccState;
  typedef mpl::list<
    smacc2::Transition<EvItemsRestocked, StCheckStains>
  > reactions;

  void onEntry()
  {
    auto & sm = this->context<SmHotelCleaner>();
    RCLCPP_INFO(getLogger(), "[Стан: StRestockItems] Доставляємо та розкладаємо подушки й простирадла...");
    sm.has_pillows_and_sheets = true;
    this->postEvent<EvItemsRestocked>();
  }
};

struct StCheckStains : smacc2::SmaccState<StCheckStains, SmHotelCleaner>
{
  using SmaccState::SmaccState;
  typedef mpl::list<
    smacc2::Transition<EvStainsFound, StCleanStains>,
    smacc2::Transition<EvNoStains, StSelectSeason>
  > reactions;

  void onEntry()
  {
    auto & sm = this->context<SmHotelCleaner>();
    RCLCPP_INFO(getLogger(), "[Стан: StCheckStains] Сканування ліжка на наявність плям...");
    if (sm.stains_remaining > 0) {
      RCLCPP_WARN(getLogger(), "  -> Виявлено плями! Перехід до очищення.");
      this->postEvent<EvStainsFound>();
    } else {
      RCLCPP_INFO(getLogger(), "  -> Ліжко чисте.");
      this->postEvent<EvNoStains>();
    }
  }
};

struct StCleanStains : smacc2::SmaccState<StCleanStains, SmHotelCleaner>
{
  using SmaccState::SmaccState;
  typedef mpl::list<
    smacc2::Transition<EvNoStains, StSelectSeason>,
    smacc2::Transition<EvCleanRetry, StCleanStains>,
    smacc2::Transition<EvCleanFailed, StSelectSeason>
  > reactions;

  void onEntry()
  {
    auto & sm = this->context<SmHotelCleaner>();
    sm.clean_attempts++;
    RCLCPP_INFO(getLogger(), "[Стан: StCleanStains] Спроба очищення #%d з 3...", sm.clean_attempts);
    sm.stains_remaining--;

    if (sm.stains_remaining <= 0) {
      RCLCPP_INFO(getLogger(), "  -> Усі плями успішно видалено!");
      this->postEvent<EvNoStains>();
    } else if (sm.clean_attempts < 3) {
      RCLCPP_WARN(getLogger(), "  -> Пляма ще залишилась, повторна спроба...");
      this->postEvent<EvCleanRetry>();
    } else {
      RCLCPP_ERROR(getLogger(), "  -> Ліміт у 3 спроби вичерпано! Потрібен виклик персоналу.");
      this->postEvent<EvCleanFailed>();
    }
  }
};

struct StSelectSeason : smacc2::SmaccState<StSelectSeason, SmHotelCleaner>
{
  using SmaccState::SmaccState;
  typedef mpl::list<
    smacc2::Transition<EvWinter, StWinterBedding>,
    smacc2::Transition<EvSummer, StSummerBedding>
  > reactions;

  void onEntry()
  {
    auto & sm = this->context<SmHotelCleaner>();
    RCLCPP_INFO(getLogger(), "[Стан: StSelectSeason] Перевірка поточного сезону: '%s'", sm.season.c_str());
    if (sm.season == "winter") {
      this->postEvent<EvWinter>();
    } else {
      this->postEvent<EvSummer>();
    }
  }
};

struct StWinterBedding : smacc2::SmaccState<StWinterBedding, SmHotelCleaner>
{
  using SmaccState::SmaccState;
  typedef mpl::list<
    smacc2::Transition<EvBeddingDone, StDone>
  > reactions;

  void onEntry()
  {
    RCLCPP_INFO(getLogger(), "[Стан: StWinterBedding] Заміна білизни та застилання теплої ЗИМОВОЇ ковдри.");
    this->postEvent<EvBeddingDone>();
  }
};

struct StSummerBedding : smacc2::SmaccState<StSummerBedding, SmHotelCleaner>
{
  using SmaccState::SmaccState;
  typedef mpl::list<
    smacc2::Transition<EvBeddingDone, StDone>
  > reactions;

  void onEntry()
  {
    RCLCPP_INFO(getLogger(), "[Стан: StSummerBedding] Заміна білизни та застилання легкої ЛІТНЬОЇ ковдри.");
    this->postEvent<EvBeddingDone>();
  }
};

struct StDone : smacc2::SmaccState<StDone, SmHotelCleaner>
{
  using SmaccState::SmaccState;

  void onEntry()
  {
    RCLCPP_INFO(getLogger(), "[Стан: StDone] Прибирання номера завершено! (Натисніть Ctrl+C для виходу)");
  }
};

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  smacc2::run<SmHotelCleaner>();
  rclcpp::shutdown();
  return 0;
}
