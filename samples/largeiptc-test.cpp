// ***************************************************************** -*- C++ -*-
// Test for large (>65535 bytes) IPTC buffer

#include <exiv2/exiv2.hpp>

#include <cassert>
#include <iostream>

int main(int argc, char *const argv[]) {
  try {
    Exiv2::XmpParser::initialize();
    ::atexit(Exiv2::XmpParser::terminate);

    if (argc != 3) {
      std::cout << "Usage: " << argv[0] << " image datafile\n";
      return 1;
    }
    std::string file(argv[1]);
    std::string data(argv[2]);

    // Read data file into data buffer
    Exiv2::FileIo io(data);
    if (io.open() != 0) {
      throw Exiv2::Error(Exiv2::kerDataSourceOpenFailed, io.path(),
                         Exiv2::strError());
    }
    Exiv2::DataBuf buf(io.size());
    std::cout << "Reading " << buf.size_ << " bytes from " << data << "\n";
    size_t readBytes = io.read(buf.pData_, buf.size_);
    if (readBytes != buf.size_ || io.error() || io.eof())
      throw Exiv2::Error(Exiv2::kerFailedToReadImageData);

    // Read metadata from file
    Exiv2::Image::UniquePtr image = Exiv2::ImageFactory::open(file);
    assert(image.get() != nullptr);
    image->readMetadata();

    // Set Preview field to the content of the data file
    Exiv2::DataValue value;
    value.read(buf.pData_, buf.size_);
    Exiv2::IptcData &iptcData = image->iptcData();
    std::cout << "IPTC fields: " << iptcData.size() << "\n";
    iptcData["Iptc.Application2.Preview"] = value;
    std::cout << "IPTC fields: " << iptcData.size() << "\n";

    // Set IRB, compare with IPTC raw data
    Exiv2::DataBuf irb = Exiv2::Photoshop::setIptcIrb(nullptr, 0, iptcData);
    std::cout << "IRB buffer : " << irb.size_ << "\n";
    const Exiv2::byte *record;
    uint32_t sizeHdr;
    uint32_t sizeData;
    Exiv2::Photoshop::locateIptcIrb(irb.pData_, irb.size_, &record, &sizeHdr,
                                    &sizeData);
    Exiv2::DataBuf rawIptc = Exiv2::IptcParser::encode(iptcData);
    std::cout << "Comparing IPTC and IRB size... ";
    if (static_cast<uint32_t>(rawIptc.size_) != sizeData) {
      std::cout << "not ";
    }
    std::cout << "ok\n";

    std::cout << "Comparing IPTC and IRB data... ";
    if (0 != memcmp(rawIptc.pData_, record + sizeHdr, sizeData)) {
      std::cout << "not ";
    }
    std::cout << "ok\n";

    // Set Iptc data and write it to the file
    image->writeMetadata();

    return EXIT_SUCCESS;
  } catch (Exiv2::AnyError &e) {
    std::cout << "Caught Exiv2 exception '" << e << "'\n";
    return EXIT_FAILURE;
  }
}
