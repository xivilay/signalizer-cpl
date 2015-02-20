#ifndef DSPWINDOWS_H
	#define DSPWINDOWS_H

	#include <cpl/Common.h>
	#include <cpl/Utility.h>
	#include <cpl/CAudioBuffer.h>
	#include <vector>
	#include <memory>

	namespace cpl
	{
		namespace dsp
		{
		
			enum class WindowTypes
			{
				Hann,
				Rectangular,
				Hamming,
				FlatTop,
				Blackman,
				Triangular,
				Parzen,
				Nutall,
				BlackmanNutall,
				BlackmanHarris,
				Gaussian,
				Slepian,
				DolphChebyshev,
				Kaiser,
				Ultraspherical,
			};

			class Window
			{
			public:
				
				enum Shape
				{
					Symmetric,
					Periodic,
					DFTEven
					
				};
				
				template<Shape symmetry, class InOutVector>
					static void generate(InOutVector & buffer, Types::fint_t N);
				
				template<Shape symmetry, class InOutVector>
					static void apply(InOutVector & buffer, Types::fint_t N);
				
				template<Shape symmetry, typename Scalar>
					Scalar operator[] (Types::fint_t K);
			private:
				WindowTypes winType;

			};
		
			namespace Windows
			{
				template<typename ScalarTy>
					class Hann : private Window
					{
					public:
					
						static const ScalarTy DFTCoeffs[3];
					
						Hann(Types::fint_t size)
						{

						}
						
						template<Shape symmetry, class InOutVector>
							static void generate(InOutVector & buffer, Types::fint_t N)
							{
							
							}
					
						template<Shape symmetry, class InOutVector>
						static void apply(InOutVector & buffer, Types::fint_t N);
					
						template<Shape symmetry>
							ScalarTy operator[] (Types::fint_t K);
					

					};

				template<typename Scalar>
					class Hamming : private Window
					{
					public:
					
						static const Scalar DFTCoeffs[3];
					
						Hamming(Types::fint_t size)
							: buffer(size)
						{

						}
						
						template<Shape symmetry, class InOutVector>
							static void generate(InOutVector & buffer, Types::fint_t N)
							{
							
							}
					
						template<Shape symmetry, class InOutVector>
						static void apply(InOutVector & buffer, Types::fint_t N);
					
						template<Shape symmetry>
							Scalar operator[] (Types::fint_t K);
					
					private:
						std::vector<Scalar> buffer;
					};

				template<typename Scalar>
					class Blackman : private Window
					{
					public:
					
						static const Scalar DFTCoeffs[5];
					
						Blackman(Types::fint_t size)
							: buffer(size)
						{

						}
						
						template<Shape symmetry, class InOutVector>
							static void generate(InOutVector & buffer, Types::fint_t N)
							{
							
							}
					
						template<Shape symmetry, class InOutVector>
						static void apply(InOutVector & buffer, Types::fint_t N);
					
						template<Shape symmetry>
							Scalar operator[] (Types::fint_t K);
					
					private:
						std::vector<Scalar> buffer;
					};

				template<typename Scalar>
					const Scalar Hann<Scalar>::DFTCoeffs[3] =
					{
						static_cast<Scalar>(-0.25),
						static_cast<Scalar>(0.5),
						static_cast<Scalar>(-0.25)
					};

				template<typename Scalar>
					const Scalar Hamming<Scalar>::DFTCoeffs[3] =
					{
						static_cast<Scalar>(-0.46),
						static_cast<Scalar>(0.54),
						static_cast<Scalar>(-0.46)
					};

				template<typename Scalar>
					const Scalar Blackman<Scalar>::DFTCoeffs[5] =
					{
						static_cast<Scalar>(-1430.0 / 18608.0),
						static_cast<Scalar>(-9240.0 / 18608.0),
						static_cast<Scalar>(7938.0/18608.0),
						static_cast<Scalar>(-9240.0/18608.0),
						static_cast<Scalar>(-1430.0/18608.0)
					};
			};
		



			std::unique_ptr<Window> windowFromString(const std::string &);
			//std::unique_ptr<Window> windowFromEnum(WindowTypes);


		/*	template<WindowTypes T>
				struct windowFromEnum
				{
					static_assert(false, "cpl::dsp::windowFromEnum<T>: T is not a supported window.");
				};*/

			template<typename Scalar, WindowTypes win>
				struct windowFromEnum;

			template<typename Scalar>
				struct windowFromEnum <Scalar, WindowTypes::Hann>
				{
					using type = Windows::Hann < Scalar > ;
				};

			template<typename Scalar>
				struct windowFromEnum <Scalar, WindowTypes::Hamming>
				{
					using type = Windows::Hamming < Scalar > ;
				};

			template<typename Scalar>
				struct windowFromEnum <Scalar, WindowTypes::Blackman>
				{
					using type = Windows::Blackman < Scalar > ;
				};

			/*template<typename Scalar>
			struct windowFromEnum < Scalar, WindowTypes::Hann >
			{

			};*/

		};
	};
#endif