#include <rclcpp/rclcpp.hpp>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <string>
#include <utility>
#include <vector>

namespace fs = std::filesystem;

struct SimConfig
{
  std::string title;
  std::string filename;
  double Kp = 5.0;
  double T = 0.050;         // 50 мс
  double a_max = 0.5;       // 0.5 рад/с^2
  double x_target = 0.05;   // 0.05 рад
  double dt_act = 0.0;      // затримка активації, с
  double dt_fb = 0.0;       // затримка зворотного зв'язку, с
  bool use_predictor = false;
  double duration = 5.0;    // тривалість симуляції, с
};

void runSimulation(const SimConfig & cfg, const std::string & out_dir, rclcpp::Logger logger)
{
  const double dt = 0.001; // крок фізичної моделі 1 мс
  const int total_steps = static_cast<int>(std::round(cfg.duration / dt));
  const int ctrl_period_steps = std::max(1, static_cast<int>(std::round(cfg.T / dt)));
  const size_t fb_delay_steps = static_cast<size_t>(std::round(cfg.dt_fb / dt));
  const size_t act_delay_steps = static_cast<size_t>(std::round(cfg.dt_act / dt));

  std::deque<std::pair<double, double>> sensor_buffer(fb_delay_steps + 1, {0.0, 0.0});
  std::deque<double> act_buffer(act_delay_steps + 1, 0.0);

  double x = 0.0;
  double v = 0.0;
  double v_cmd_current = 0.0;

  double max_x = 0.0;
  double min_x_after_peak = cfg.x_target;
  bool reached_target = false;

  std::ofstream csv(out_dir + "/" + cfg.filename);
  csv << "t,x,v,v_cmd\n";
  csv << std::fixed << std::setprecision(6);

  for (int k = 0; k <= total_steps; ++k) {
    const double t = k * dt;

    // Запис поточного стану в буфер затримки енкодера
    sensor_buffer.push_back({x, v});
    if (sensor_buffer.size() > fb_delay_steps + 1) {
      sensor_buffer.pop_front();
    }
    const auto [x_meas, v_meas] = sensor_buffer.front();

    // Виконання цифрового контролера з періодом квантування T
    if (k % ctrl_period_steps == 0) {
      const double dt_loop = cfg.dt_act + cfg.dt_fb;
      const double x_eval = cfg.use_predictor ? (x_meas + v_meas * dt_loop) : x_meas;
      const double e = cfg.x_target - x_eval;
      v_cmd_current = cfg.Kp * e;
    }

    // Запис команди в буфер затримки актуації
    act_buffer.push_back(v_cmd_current);
    if (act_buffer.size() > act_delay_steps + 1) {
      act_buffer.pop_front();
    }
    const double v_cmd_applied = act_buffer.front();

    // Збереження точки у CSV кожні 10 мс
    if (k % 10 == 0) {
      csv << t << "," << x << "," << v << "," << v_cmd_applied << "\n";
    }

    if (x > max_x) {
      max_x = x;
    }
    if (x >= cfg.x_target) {
      reached_target = true;
    }
    if (reached_target && t > 1.5 && x < min_x_after_peak) {
      min_x_after_peak = x;
    }

    // Інтегрування руху приводу з обмеженням прискорення/гальмування a_max
    if (k < total_steps) {
      const double a_req = (v_cmd_applied - v) / dt;
      const double a = std::clamp(a_req, -cfg.a_max, cfg.a_max);
      const double v_next = v + a * dt;
      const double x_next = x + 0.5 * (v + v_next) * dt;
      v = v_next;
      x = x_next;
    }
  }

  csv.close();

  const double overshoot_pct = std::max(0.0, (max_x - cfg.x_target) / cfg.x_target * 100.0);
  const double steady_error = std::abs(cfg.x_target - x);

  RCLCPP_INFO(
    logger,
    "[%s] x_max=%.5f рад (перерегулювання: %.1f%%), x_кінцеве=%.5f рад, |e_кінц|=%.5f рад",
    cfg.title.c_str(), max_x, overshoot_pct, x, steady_error);
}

int main(int argc, char ** argv)
{
  rclcpp::init(argc, argv);
  auto node = rclcpp::Node::make_shared("joint_p_controller_sim");
  auto logger = node->get_logger();

  const char * home = std::getenv("HOME");
  std::string out_dir = std::string(home ? home : "/tmp") + "/ros2_ws/sim_results";
  fs::create_directories(out_dir);

  RCLCPP_INFO(logger, "=== Симуляція П-регулятора суглоба (результати у %s) ===", out_dir.c_str());

  std::vector<SimConfig> scenarios = {
    {"1. Без затримок (dt_loop=0 мс)", "case1_no_delay.csv", 5.0, 0.050, 0.5, 0.05, 0.000, 0.000, false, 5.0},
    {"2. Із затримкою (dt_loop=150 мс)", "case2_with_delay.csv", 5.0, 0.050, 0.5, 0.05, 0.100, 0.050, false, 5.0},
    {"3. Лінійний предиктор (dt_loop=150 мс)", "case3_compensated.csv", 5.0, 0.050, 0.5, 0.05, 0.100, 0.050, true, 5.0},
    {"4. Варіація Kp=2.0 (dt_loop=150 мс)", "var_kp_2.csv", 2.0, 0.050, 0.5, 0.05, 0.100, 0.050, false, 5.0},
    {"5. Варіація Kp=10.0 (dt_loop=150 мс)", "var_kp_10.csv", 10.0, 0.050, 0.5, 0.05, 0.100, 0.050, false, 5.0},
    {"6. Варіація T=150 мс (dt_loop=150 мс)", "var_T_150ms.csv", 5.0, 0.150, 0.5, 0.05, 0.100, 0.050, false, 5.0},
    {"7. Варіація dt_loop=300 мс (Kp=5.0)", "var_delay_300ms.csv", 5.0, 0.050, 0.5, 0.05, 0.200, 0.100, false, 5.0}
  };

  for (const auto & sc : scenarios) {
    runSimulation(sc, out_dir, logger);
  }

  RCLCPP_INFO(logger, "=== Усі CSV-файли успішно сформовано! ===");
  rclcpp::shutdown();
  return 0;
}
