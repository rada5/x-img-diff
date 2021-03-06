#include <opencv2/opencv.hpp>
#include <iostream>
#include "hunter.hpp"
#include "rectutil.hpp"
#include "args/args.hxx"

using namespace std;
using namespace cv;

int main(int argc, char* argv[], char* envp[]) {

  args::ArgumentParser parser("Compare two images");
  args::HelpFlag help(parser, "help", "Display this help menu.", {'h', "help"});
  args::Flag verbose(parser, "verbose", "Display debug logging messages.", {'v', "verbose"});
  args::ValueFlag<int> shiftDelta(parser, "integer", "Minimal translation distance in matching each area.", {"shift-delta"});
  args::ValueFlag<int> maxMatchingPoints(parser, "integer", "Maximum value of matching points in clustering.", {"max-matching-points"});
  args::Flag noCanny(parser, "noCanny", "Disable preprocess using Canny filter.", {"no-canny"});
  args::Positional<string> actual(parser, "actual", "Actual image path");
  args::Positional<string> expected(parser, "expected", "Expected image path");
  args::Positional<string> outPath(parser, "out", "Output image path");

  try {
    parser.ParseCLI(argc, argv);
  } catch (args::Help) {
    std::cout << parser;
    return 0;
  } catch (args::ParseError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  } catch (args::ValidationError e) {
    std::cerr << e.what() << std::endl;
    std::cerr << parser;
    return 1;
  }

  auto img1 = imread(args::get(actual), IMREAD_COLOR);
  auto img2 = imread(args::get(expected), IMREAD_COLOR);

  if (!img1.data) {
    cerr << "No actual image data" << endl;
    return -1;
  }

  if (!img2.data) {
    cerr << "No expected image data" << endl;
    return -1;
  }

  auto conf = ph::DiffConfig();

  conf.debug = verbose;
  conf.useCanny = !noCanny;
  if (shiftDelta) conf.shiftDelta = args::get(shiftDelta);
  if (maxMatchingPoints) conf.maxMatchingPoints = args::get(maxMatchingPoints);

  ph::DiffResult result;
  ph::detectDiff(img1, img2, result, conf);

  auto outImg1 = img1.clone();
  auto outImg2 = img2.clone();
  auto irects1 = vector<Rect>(), irects2 = vector<Rect>();
  auto urects1 = vector<Rect>(), urects2 = vector<Rect>();
  for (auto& pr: result.matches) {
    irects1.push_back(pr.center1);
    urects1.push_back(pr.bounding1);
    irects2.push_back(pr.center2);
    urects2.push_back(pr.bounding2);
    if (!pr.isMatched) {
      ph::rectu::drawRects(outImg1, pr.diffMarkers1, Scalar(0, 0, 255), 2);
      ph::rectu::drawRects(outImg2, pr.diffMarkers2, Scalar(0, 0, 255), 2);
    }
  }
  ph::rectu::drawRects(outImg1, result.strayingRects1, Scalar(200, 100, 200), 2);
  ph::rectu::drawRects(outImg2, result.strayingRects2, Scalar(200, 100, 200), 2);
  // ph::rectu::drawRects(outImg1, irects1, Scalar(0, 255, 0), 1);
  // ph::rectu::drawRects(outImg2, irects2, Scalar(0, 255, 0), 1);
  ph::rectu::drawRects(outImg1, urects1, Scalar(200, 200, 100), 1);
  ph::rectu::drawRects(outImg2, urects2, Scalar(200, 200, 100), 1);
  Mat outImg;
  drawMatches(outImg1, vector<KeyPoint>(), outImg2, vector<KeyPoint>(), vector<DMatch>(), outImg, Scalar(100, 100, 100));
  // imshow("rects2", outImg);
  if (args::get(outPath).length() > 0) {
    imwrite(args::get(outPath), outImg);
  }
  return 0;
}
