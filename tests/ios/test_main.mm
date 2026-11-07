/**
 * @file test_main.mm
 * @brief End-to-end iOS tests consuming the packaged ascend-forge XCFramework.
 */

#import <UIKit/UIKit.h>
#import <os/log.h>

#include <ascend_forge/version.hpp>

#include "arena_tests.h"
#include "utf8_tests.h"

#include <string>
#include <string_view>

namespace
{

os_log_t TestLog()
{
    static os_log_t log = os_log_create("com.example.ascendforge.test", "tests");
    return log;
}

class TestRun
{
  public:
    void Check(bool condition, std::string_view message)
    {
        const std::string text{message};
        if (condition)
        {
            os_log_info(TestLog(), "PASS: %{public}s", text.c_str());
            return;
        }
        ++failures_;
        os_log_error(TestLog(), "FAIL: %{public}s", text.c_str());
    }

    [[nodiscard]] int failures() const noexcept
    {
        return failures_;
    }

  private:
    int failures_ = 0;
};

int RunTests()
{
    TestRun run;
    const std::string_view version = ascend_forge::Version();
    run.Check(version == ASCEND_FORGE_EXPECTED_VERSION,
              "XCFramework exports the VERSION-derived API");
    const std::string major_prefix = std::to_string(ascend_forge::kVersionMajor) + ".";
    run.Check(version.substr(0, major_prefix.size()) == major_prefix,
              "Version() matches the compiled major version");
    run.Check(ascend_forge::kVersionMajor >= 0, "major version is non-negative");
    run.Check(ArenaTests_Run() == 0, "arena checks passed");
    run.Check(Utf8Tests_Run() == 0, "utf8 checks passed");
    return run.failures();
}

} // namespace

@interface TestAppDelegate : UIResponder <UIApplicationDelegate>
@property(nonatomic, strong) UIWindow *window;
@end

@implementation TestAppDelegate

- (BOOL)application:(UIApplication *)application
    didFinishLaunchingWithOptions:(NSDictionary<UIApplicationLaunchOptionsKey, id> *)launchOptions
{
    (void)application;
    (void)launchOptions;
    self.window = [[UIWindow alloc] initWithFrame:UIScreen.mainScreen.bounds];
    self.window.rootViewController = [[UIViewController alloc] init];
    self.window.rootViewController.view.backgroundColor = UIColor.systemBackgroundColor;
    [self.window makeKeyAndVisible];

    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
      const int failures = RunTests();
      os_log_info(TestLog(), "ASCEND_FORGE_IOS_TEST_RESULT: %{public}d", failures);
    });
    return YES;
}

@end

int main(int argc, char *argv[])
{
    @autoreleasepool
    {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass(TestAppDelegate.class));
    }
}
