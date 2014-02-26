require "formula"

class Ubertooth < Formula
  homepage "https://github.com/greatscottgadgets/ubertooth"
  url "https://github.com/greatscottgadgets/ubertooth/archive/2014-02-R2.tar.gz"
  sha1 "773eab8b6440dcf7fd6bcc83d9e365c3239dd5f3"
  version "2014-02-R2"

  head "https://github.com/greatscottgadgets/ubertooth.git"

  resource "pyusb" do
    url "https://pypi.python.org/packages/source/p/pyusb/pyusb-1.0.0b1.tar.gz"
    sha1 "f0ca8bdfbb59645ea73976f9a3cb2b0701667148"
  end

  depends_on "cmake" => :build
  depends_on :python
  depends_on "libbtbb"

  def install
    ENV.prepend_create_path "PYTHONPATH", libexec+"lib/python2.7/site-packages"
    resource("pyusb").stage { system "python", "setup.py", "install", "--prefix=#{libexec}" }

    mkdir "host/build" do
      system "cmake", "..", *std_cmake_args
      system "make", "install"
    end
  end
end
