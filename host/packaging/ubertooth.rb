require "formula"

class Ubertooth < Formula
  homepage "https://github.com/greatscottgadgets/ubertooth"
  url "https://github.com/greatscottgadgets/ubertooth/archive/2014-02-R1.tar.gz"
  sha1 "1ab0d89a41f5443f3d65ebb9e05c3e5338de2ba4"
  version "2014-02-R1"

  head "https://github.com/greatscottgadgets/ubertooth.git"

  resource "pyusb" do
    url "https://pypi.python.org/packages/source/p/pyusb/pyusb-1.0.0b1.tar.gz"
    sha256 "6fa787840baa8c6a041e370bf381127aae5fb44c820ba655f966b7da4de6279f"
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
